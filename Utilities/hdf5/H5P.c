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

/* Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:  Generic Property Functions
 */

#define H5P_PACKAGE    /*suppress error about including H5Ppkg    */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC  H5P_init_interface


/* Private header files */
#include "H5private.h"    /* Generic Functions      */
#include "H5Dprivate.h"    /* Datasets        */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Fprivate.h"    /* Files                */
#include "H5FLprivate.h"  /* Free lists                           */
#include "H5Iprivate.h"    /* IDs            */
#include "H5MMprivate.h"  /* Memory management      */
#include "H5Ppkg.h"    /* Property lists        */

/* Local macros */
#define H5P_DEFAULT_SKIPLIST_HEIGHT     8

/* Local variables */

/*
 * Predefined property list classes. These are initialized at runtime by
 * H5P_init_interface() in this source file.
 */
hid_t H5P_CLS_NO_CLASS_g            = FAIL;
hid_t H5P_CLS_FILE_CREATE_g         = FAIL;
hid_t H5P_CLS_FILE_ACCESS_g         = FAIL;
hid_t H5P_CLS_DATASET_CREATE_g      = FAIL;
hid_t H5P_CLS_DATASET_XFER_g        = FAIL;
hid_t H5P_CLS_MOUNT_g               = FAIL;

/*
 * Predefined property lists for each predefined class. These are initialized
 * at runtime by H5P_init_interface() in this source file.
 */
hid_t H5P_LST_NO_CLASS_g            = FAIL;
hid_t H5P_LST_FILE_CREATE_g         = FAIL;
hid_t H5P_LST_FILE_ACCESS_g         = FAIL;
hid_t H5P_LST_DATASET_CREATE_g      = FAIL;
hid_t H5P_LST_DATASET_XFER_g        = FAIL;
hid_t H5P_LST_MOUNT_g               = FAIL;

/* Track the revision count of a class, to make comparisons faster */
static unsigned H5P_next_rev=0;
#define H5P_GET_NEXT_REV        (H5P_next_rev++)

/* Declare a free list to manage the H5P_genprop_t struct */
H5FL_DEFINE(H5P_genprop_t);

/* Declare a free list to manage the H5P_genplist_t struct */
H5FL_DEFINE(H5P_genplist_t);

/* Declare a free list to manage the H5P_genclass_t struct */
H5FL_DEFINE_STATIC(H5P_genclass_t);

/* Local typedefs */

/* Typedef for checking for duplicate class names in parent class */
typedef struct {
    const H5P_genclass_t *parent;       /* Pointer to parent class */
    const char *name;                   /* Pointer to name to check */
} H5P_check_class_t;

/* Local static functions */
static H5P_genclass_t *H5P_create_class(H5P_genclass_t *par_class,
     const char *name, unsigned internal,
     H5P_cls_create_func_t cls_create, void *create_data,
     H5P_cls_copy_func_t cls_copy, void *copy_data,
     H5P_cls_close_func_t cls_close, void *close_data);
static herr_t H5P_unregister(H5P_genclass_t *pclass, const char *name);
static H5P_genprop_t *H5P_dup_prop(H5P_genprop_t *oprop, H5P_prop_within_t type);
static herr_t H5P_free_prop(H5P_genprop_t *prop);


/*--------------------------------------------------------------------------
 NAME
    H5P_do_prop_cb1
 PURPOSE
    Internal routine to call a property list callback routine and update
    the property list accordingly.
 USAGE
    herr_t H5P_do_prop_cb1(slist,prop,cb)
        H5SL_t *slist;          IN/OUT: Skip list to hold changed properties
        H5P_genprop_t *prop;    IN: Property to call callback for
        H5P_prp_cb1_t *cb;      IN: Callback routine to call
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Calls the callback routine passed in.  If the callback routine changes
    the property value, then the property is duplicated and added to skip list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_do_prop_cb1(H5SL_t *slist, H5P_genprop_t *prop, H5P_prp_cb1_t cb)
{
    void *tmp_value=NULL;       /* Temporary value buffer */
    H5P_genprop_t *pcopy=NULL;  /* Copy of property to insert into skip list */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_do_prop_cb1);

    /* Allocate space for a temporary copy of the property value */
    if (NULL==(tmp_value=H5MM_malloc(prop->size)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for temporary property value");
    HDmemcpy(tmp_value,prop->value,prop->size);

    /* Call "type 1" callback ('create', 'copy' or 'close') */
    if(cb(prop->name,prop->size,tmp_value)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL,"Property callback failed");

    /* Check if the property value changed */
    if(HDmemcmp(tmp_value,prop->value,prop->size)) {
        /* Make a copy of the class's property */
        if((pcopy=H5P_dup_prop(prop,H5P_PROP_WITHIN_LIST))==NULL)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property");

        /* Copy the changed value into the new property */
        HDmemcpy(pcopy->value,tmp_value,prop->size);

        /* Insert the changed property into the property list */
        if(H5P_add_prop(slist,pcopy)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into skip list");
    } /* end if */

done:
    /* Release the temporary value buffer */
    if(tmp_value!=NULL)
        H5MM_xfree(tmp_value);

    /* Cleanup on failure */
    if(ret_value<0) {
        if(pcopy!=NULL)
            H5P_free_prop(pcopy);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5P_do_prop_cb1() */


/*-------------------------------------------------------------------------
 * Function:  H5P_init
 *
 * Purpose:  Initialize the interface from some other layer.
 *
 * Return:  Success:  non-negative
 *
 *    Failure:  negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 4, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5P_init(void)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5P_init, FAIL);
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
NAME
   H5P_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5P_init_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
static herr_t
H5P_init_interface(void)
{
    H5P_genclass_t  *root_class;    /* Pointer to root property list class created */
    H5P_genclass_t  *pclass;        /* Pointer to property list class to create */
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT(H5P_init_interface);

    /*
     * Initialize the Generic Property class & object groups.
     */
    if (H5I_init_group(H5I_GENPROP_CLS, H5I_GENPROPCLS_HASHSIZE, 0, (H5I_free_t)H5P_close_class) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTINIT, FAIL, "unable to initialize atom group");
    if (H5I_init_group(H5I_GENPROP_LST, H5I_GENPROPOBJ_HASHSIZE, 0, (H5I_free_t)H5P_close) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTINIT, FAIL, "unable to initialize atom group");

    /* Create root property list class */

    /* Allocate the root class */
    assert(H5P_CLS_NO_CLASS_g==(-1));
    if (NULL==(root_class = H5P_create_class (NULL,"root",1,NULL,NULL,NULL,NULL,NULL,NULL)))
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL, "class initialization failed");

    /* Register the root class */
    if ((H5P_CLS_NO_CLASS_g = H5I_register (H5I_GENPROP_CLS, root_class))<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register property list class");

    /* Register the file creation and file access property classes */

    /* Allocate the file creation class */
    assert(H5P_CLS_FILE_CREATE_g==(-1));
    if (NULL==(pclass = H5P_create_class (root_class,"file create",1,NULL,NULL,NULL,NULL,NULL,NULL)))
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL, "class initialization failed");

    /* Register the file creation class */
    if ((H5P_CLS_FILE_CREATE_g = H5I_register (H5I_GENPROP_CLS, pclass))<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register property list class");

    /* Allocate the file access class */
    assert(H5P_CLS_FILE_ACCESS_g==(-1));
    if (NULL==(pclass = H5P_create_class (root_class,"file access",1,H5F_acs_create,NULL,H5F_acs_copy,NULL,H5F_acs_close,NULL)))
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL, "class initialization failed");

    /* Register the file access class */
    if ((H5P_CLS_FILE_ACCESS_g = H5I_register (H5I_GENPROP_CLS, pclass))<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register property list class");

    /* Register the dataset creation and data xfer property classes */

    /* Allocate the dataset creation class */
    assert(H5P_CLS_DATASET_CREATE_g==(-1));
    if (NULL==(pclass = H5P_create_class (root_class,"dataset create",1,NULL,NULL,H5D_crt_copy,NULL,H5D_crt_close,NULL)))
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL, "class initialization failed");

    /* Register the dataset creation class */
    if ((H5P_CLS_DATASET_CREATE_g = H5I_register (H5I_GENPROP_CLS, pclass))<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register property list class");

    /* Allocate the data xfer class */
    assert(H5P_CLS_DATASET_XFER_g==(-1));
    if (NULL==(pclass = H5P_create_class (root_class,"data xfer",1,H5D_xfer_create,NULL,H5D_xfer_copy,NULL,H5D_xfer_close,NULL)))
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL, "class initialization failed");

    /* Register the data xfer class */
    if ((H5P_CLS_DATASET_XFER_g = H5I_register (H5I_GENPROP_CLS, pclass))<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register property list class");

    /* Allocate the mount class */
    assert(H5P_CLS_MOUNT_g==(-1));
    if (NULL==(pclass = H5P_create_class (root_class,"file mount",1,NULL,NULL,NULL,NULL,NULL,NULL)))
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL, "class initialization failed");

    /* Register the mount class */
    if ((H5P_CLS_MOUNT_g = H5I_register (H5I_GENPROP_CLS, pclass))<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTREGISTER, FAIL, "can't register property list class");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5P_term_interface
 PURPOSE
    Terminate various H5P objects
 USAGE
    void H5P_term_interface()
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
    Release the atom group and any other resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5P_term_interface(void)
{
    int  nlist=0;
    int  nclass=0;
    int  n=0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_term_interface);

    if (H5_interface_initialize_g) {
        /* Destroy HDF5 library property classes & lists */

        /* Check if there are any open property list classes or lists */
        nclass = H5I_nmembers(H5I_GENPROP_CLS);
        nlist = H5I_nmembers(H5I_GENPROP_LST);
        n=nclass+nlist;

        /* If there are any open classes or groups, attempt to get rid of them. */
        if (n) {
            /* Clear the lists */
            if(nlist>0) {
                H5I_clear_group(H5I_GENPROP_LST, FALSE);

                /* Reset the default property lists, if they've been closed */
                if(H5I_nmembers(H5I_GENPROP_LST)==0) {
                    H5P_LST_NO_CLASS_g =
                        H5P_LST_FILE_CREATE_g =
                        H5P_LST_FILE_ACCESS_g =
                        H5P_LST_DATASET_CREATE_g =
                        H5P_LST_DATASET_XFER_g =
                        H5P_LST_MOUNT_g = (-1);
                } /* end if */
            } /* end if */

            /* Only attempt to close the classes after all the lists are closed */
            if(nlist==0 && nclass>0) {
                H5I_clear_group(H5I_GENPROP_CLS, FALSE);

                /* Reset the default property lists, if they've been closed */
                if(H5I_nmembers(H5I_GENPROP_CLS)==0) {
                    H5P_CLS_NO_CLASS_g =
                        H5P_CLS_FILE_CREATE_g =
                        H5P_CLS_FILE_ACCESS_g =
                        H5P_CLS_DATASET_CREATE_g =
                        H5P_CLS_DATASET_XFER_g =
                        H5P_CLS_MOUNT_g = (-1);
                } /* end if */
            } /* end if */
        } else {
            H5I_destroy_group(H5I_GENPROP_LST);
            n++; /*H5I*/
            H5I_destroy_group(H5I_GENPROP_CLS);
            n++; /*H5I*/

            H5_interface_initialize_g = 0;
        }
    }
    FUNC_LEAVE_NOAPI(n);
}


/*--------------------------------------------------------------------------
 NAME
    H5P_copy_pclass
 PURPOSE
    Internal routine to copy a generic property class
 USAGE
    hid_t H5P_copy_pclass(pclass)
        H5P_genclass_t *pclass;      IN: Property class to copy
 RETURNS
    Success: valid property class ID on success (non-negative)
    Failure: negative
 DESCRIPTION
    Copy a property class and return the ID.  This routine does not make
    any callbacks.  (They are only make when operating on property lists).

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genclass_t *
H5P_copy_pclass(H5P_genclass_t *pclass)
{
    H5P_genclass_t *new_pclass = NULL;  /* Property list class copied */
    H5P_genprop_t *pcopy;               /* Copy of property to insert into class */
    H5P_genclass_t *ret_value=NULL;     /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_copy_pclass);

    assert(pclass);

    /*
     * Create new property class object
     */

    /* Create the new property list class */
    if (NULL==(new_pclass=H5P_create_class(pclass->parent, pclass->name, 0, pclass->create_func, pclass->create_data, pclass->copy_func, pclass->copy_data, pclass->close_func, pclass->close_data)))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, NULL, "unable to create property list class");

    /* Copy the properties registered for this class */
    if(pclass->nprops>0) {
        H5SL_node_t *curr_node;   /* Current node in skip list */

        /* Walk through the properties in the old class */
        curr_node=H5SL_first(pclass->props);
        while(curr_node!=NULL) {
            /* Make a copy of the class's property */
            if((pcopy=H5P_dup_prop(H5SL_item(curr_node),H5P_PROP_WITHIN_CLASS))==NULL)
                HGOTO_ERROR (H5E_PLIST, H5E_CANTCOPY, NULL,"Can't copy property");

            /* Insert the initialized property into the property list */
            if(H5P_add_prop(new_pclass->props,pcopy)<0)
                HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, NULL,"Can't insert property into class");

            /* Increment property count for class */
            new_pclass->nprops++;

            /* Get the next property node in the list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end if */

    /* Set the return value */
    ret_value=new_pclass;

done:
    if (ret_value==NULL && new_pclass)
        H5P_close_class(new_pclass);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_copy_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5P_copy_plist
 PURPOSE
    Internal routine to copy a generic property list
 USAGE
        hid_t H5P_copy_plist(old_plist_id)
            hid_t old_plist_id;             IN: Property list ID to copy
 RETURNS
    Success: valid property list ID on success (non-negative)
    Failure: negative
 DESCRIPTION
    Copy a property list and return the ID.  This routine calls the
    class 'copy' callback after any property 'copy' callbacks are called
    (assuming all property 'copy' callbacks return successfully).

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5P_copy_plist(H5P_genplist_t *old_plist)
{
    H5P_genclass_t *tclass;     /* Temporary class pointer */
    H5P_genplist_t *new_plist=NULL;  /* New property list generated from copy */
    H5P_genprop_t *tmp;         /* Temporary pointer to properties */
    H5P_genprop_t *new_prop;    /* New property created for copy */
    hid_t new_plist_id;         /* Property list ID of new list created */
    H5SL_node_t *curr_node;     /* Current node in skip list */
    H5SL_t *seen=NULL;          /* Skip list containing properties already seen */
    size_t nseen;               /* Number of items 'seen' */
    hbool_t has_parent_class;   /* Flag to indicate that this property list's class has a parent */
    hid_t ret_value=FAIL;       /* return value */

    FUNC_ENTER_NOAPI(H5P_copy_plist, FAIL);

    assert(old_plist);

    /*
     * Create new property list object
     */

    /* Allocate room for the property list */
    if (NULL==(new_plist = H5FL_CALLOC(H5P_genplist_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,"memory allocation failed");

    /* Set class state */
    new_plist->pclass = old_plist->pclass;
    new_plist->nprops = 0;      /* Initially the plist has the same number of properties as the class */
    new_plist->class_init = 0;  /* Initially, wait until the class callback finishes to set */

    /* Initialize the skip list to hold the changed properties */
    if((new_plist->props=H5SL_create(H5SL_TYPE_STR,0.5,H5P_DEFAULT_SKIPLIST_HEIGHT))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,FAIL,"can't create skip list for changed properties");

    /* Create the skip list for deleted properties */
    if((new_plist->del=H5SL_create(H5SL_TYPE_STR,0.5,H5P_DEFAULT_SKIPLIST_HEIGHT))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,FAIL,"can't create skip list for deleted properties");

    /* Create the skip list to hold names of properties already seen
     * (This prevents a property in the class hierarchy from having it's
     * 'create' callback called, if a property in the class hierarchy has
     * already been seen)
     */
    if((seen=H5SL_create(H5SL_TYPE_STR,0.5,H5P_DEFAULT_SKIPLIST_HEIGHT))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,FAIL,"can't create skip list for seen properties");
    nseen=0;

    /* Cycle through the deleted properties & copy them into the new list's deleted section */
    if(H5SL_count(old_plist->del)>0) {
        curr_node=H5SL_first(old_plist->del);
        while(curr_node) {
            char *new_name;   /* Pointer to new name */

            /* Duplicate string for insertion into new deleted property skip list */
            if((new_name=H5MM_xstrdup((char *)H5SL_item(curr_node)))==NULL)
                HGOTO_ERROR(H5E_RESOURCE,H5E_NOSPACE,FAIL,"memory allocation failed");

            /* Insert property name into deleted list */
            if(H5SL_insert(new_plist->del,new_name,new_name)<0)
                HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into deleted skip list");

            /* Add property name to "seen" list */
            if(H5SL_insert(seen,new_name,new_name)<0)
                HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list");
            nseen++;

            /* Get the next property node in the skip list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end if */

    /* Cycle through the properties and copy them also */
    if(H5SL_count(old_plist->props)>0) {
        curr_node=H5SL_first(old_plist->props);
        while(curr_node) {
            /* Get a pointer to the node's property */
            tmp=H5SL_item(curr_node);

            /* Make a copy of the list's property */
            if((new_prop=H5P_dup_prop(tmp,H5P_PROP_WITHIN_LIST))==NULL)
                HGOTO_ERROR (H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property");

            /* Call property copy callback, if it exists */
            if(new_prop->copy) {
                if((new_prop->copy)(new_prop->name,new_prop->size,new_prop->value)<0) {
                    H5P_free_prop(new_prop);
                    HGOTO_ERROR (H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property");
                } /* end if */
            } /* end if */

            /* Insert the initialized property into the property list */
            if(H5P_add_prop(new_plist->props,new_prop)<0) {
                H5P_free_prop(new_prop);
                HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into list");
            } /* end if */

            /* Add property name to "seen" list */
            if(H5SL_insert(seen,new_prop->name,new_prop->name)<0)
                HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list");
            nseen++;

            /* Increment the number of properties in list */
            new_plist->nprops++;

            /* Get the next property node in the skip list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end if */

    /*
     * Check for copying class properties (up through list of parent classes also),
     * initialize each with default value & make property 'copy' callback.
     */
    tclass=old_plist->pclass;
    has_parent_class=(tclass!=NULL && tclass->parent!=NULL && tclass->parent->nprops>0);
    while(tclass!=NULL) {
        if(tclass->nprops>0) {
            /* Walk through the properties in the old class */
            curr_node=H5SL_first(tclass->props);
            while(curr_node!=NULL) {
                /* Get pointer to property from node */
                tmp=H5SL_item(curr_node);

                /* Only "copy" properties we haven't seen before */
                if(nseen==0 || H5SL_search(seen,tmp->name)==NULL) {
                    /* Call property creation callback, if it exists */
                    if(tmp->copy) {
                        /* Call the callback & insert changed value into skip list (if necessary) */
                        if(H5P_do_prop_cb1(new_plist->props,tmp,tmp->copy)<0)
                            HGOTO_ERROR (H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't create property");
                    } /* end if */

                    /* Add property name to "seen" list, if we have other classes to work on */
                    if(has_parent_class) {
                        if(H5SL_insert(seen,tmp->name,tmp->name)<0)
                            HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list");
                        nseen++;
                    } /* end if */

                    /* Increment the number of properties in list */
                    new_plist->nprops++;
                } /* end if */

                /* Get the next property node in the skip list */
                curr_node=H5SL_next(curr_node);
            } /* end while */
        } /* end if */

        /* Go up to parent class */
        tclass=tclass->parent;
    } /* end while */

    /* Increment the number of property lists derived from class */
    if(H5P_access_class(new_plist->pclass,H5P_MOD_INC_LST)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL,"Can't increment class ref count");

    /* Get an atom for the property list */
    if ((new_plist_id = H5I_register(H5I_GENPROP_LST, new_plist))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to atomize property list");

    /* Save the property list ID in the property list struct, for use in the property class's 'close' callback */
    new_plist->plist_id=new_plist_id;

    /* Call the class callback (if it exists) now that we have the property list ID */
    if(new_plist->pclass->copy_func!=NULL) {
        if((new_plist->pclass->copy_func)(new_plist_id,old_plist->plist_id,old_plist->pclass->copy_data)<0) {
            /* Delete ID, ignore return value */
            H5I_remove(new_plist_id);
            HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL,"Can't initialize property");
        } /* end if */
    } /* end if */

    /* Set the class initialization flag */
    new_plist->class_init=1;

    /* Set the return value */
    ret_value=new_plist_id;

done:
    /* Release the list of 'seen' properties */
    if(seen!=NULL)
        H5SL_close(seen);

    if (ret_value<0 && new_plist)
        H5P_close(new_plist);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_copy_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5Pcopy
 PURPOSE
    Routine to copy a property list or class
 USAGE
    hid_t H5Pcopy(id)
        hid_t id;           IN: Property list or class ID to copy
 RETURNS
    Success: valid property list ID on success (non-negative)
    Failure: negative
 DESCRIPTION
    Copy a property list or class and return the ID.  This routine calls the
    class 'copy' callback after any property 'copy' callbacks are called
    (assuming all property 'copy' callbacks return successfully).

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5Pcopy(hid_t id)
{
    void *obj;                 /* Property object to copy */
    hid_t ret_value=FALSE;      /* return value */

    FUNC_ENTER_API(H5Pcopy, FAIL);
    H5TRACE1("i","i",id);

    if (H5P_DEFAULT==id)
        HGOTO_DONE(H5P_DEFAULT);

    /* Check arguments. */
    if (H5I_GENPROP_LST != H5I_get_type(id) && H5I_GENPROP_CLS != H5I_get_type(id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not property object");
    if(NULL == (obj = H5I_object(id)))
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property object doesn't exist");

    /* Compare property lists */
    if(H5I_GENPROP_LST == H5I_get_type(id)) {
        if((ret_value=H5P_copy_plist(obj))<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL, "can't copy property list");
    } /* end if */
    /* Must be property classes */
    else {
        H5P_genclass_t *copy_class;      /* Copy of class */

        /* Copy the class */
        if((copy_class=H5P_copy_pclass(obj))==NULL)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL, "can't copy property class");

        /* Get an atom for the copied class */
        if ((ret_value = H5I_register(H5I_GENPROP_CLS, copy_class))<0) {
            H5P_close_class(copy_class);
            HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to atomize property list class");
        } /* end if */
    } /* end else */

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pcopy() */


/*--------------------------------------------------------------------------
 NAME
    H5P_dup_prop
 PURPOSE
    Internal routine to duplicate a property
 USAGE
    H5P_genprop_t *H5P_dup_prop(oprop)
        H5P_genprop_t *oprop;   IN: Pointer to property to copy
        H5P_prop_within_t type; IN: Type of object the property will be inserted into
 RETURNS
    Returns a pointer to the newly created duplicate of a property on success,
        NULL on failure.
 DESCRIPTION
    Allocates memory and copies property information into a new property object.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genprop_t *
H5P_dup_prop(H5P_genprop_t *oprop, H5P_prop_within_t type)
{
    H5P_genprop_t *prop=NULL;        /* Pointer to new property copied */
    H5P_genprop_t *ret_value;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_dup_prop);

    assert(oprop);
    assert(type!=H5P_PROP_WITHIN_UNKNOWN);

    /* Allocate the new property */
    if (NULL==(prop = H5FL_MALLOC (H5P_genprop_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Copy basic property information */
    HDmemcpy(prop,oprop,sizeof(H5P_genprop_t));

    /* Check if we should duplicate the name or share it */

    /* Duplicating property for a class */
    if(type==H5P_PROP_WITHIN_CLASS) {
        assert(oprop->type==H5P_PROP_WITHIN_CLASS);
        assert(oprop->shared_name==0);

        /* Duplicate name */
        prop->name = H5MM_xstrdup(oprop->name);
    } /* end if */
    /* Duplicating property for a list */
    else {
        /* Check if we are duplicating a property from a list or a class */

        /* Duplicating a property from a list */
        if(oprop->type==H5P_PROP_WITHIN_LIST) {
            /* If the old property's name wasn't shared, we have to copy it here also */
            if(!oprop->shared_name)
                prop->name = H5MM_xstrdup(oprop->name);
        } /* end if */
        /* Duplicating a property from a class */
        else {
            assert(oprop->type==H5P_PROP_WITHIN_CLASS);
            assert(oprop->shared_name==0);

            /* Share the name */
            prop->shared_name=1;

            /* Set the type */
            prop->type=type;
        } /* end else */
    } /* end else */

    /* Duplicate current value, if it exists */
    if(oprop->value!=NULL) {
        assert(prop->size>0);
        if (NULL==(prop->value = H5MM_malloc (prop->size)))
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
        HDmemcpy(prop->value,oprop->value,prop->size);
    } /* end if */

    /* Set return value */
    ret_value=prop;

done:
    /* Free any resources allocated */
    if(ret_value==NULL) {
        if(prop!=NULL) {
            if(prop->name!=NULL)
                H5MM_xfree(prop->name);
            if(prop->value!=NULL)
                H5MM_xfree(prop->value);
            H5FL_FREE(H5P_genprop_t,prop);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_dup_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_create_prop
 PURPOSE
    Internal routine to create a new property
 USAGE
    H5P_genprop_t *H5P_create_prop(name,size,type,value,prp_create,prp_set,
            prp_get,prp_delete,prp_close)
        const char *name;       IN: Name of property to register
        size_t size;            IN: Size of property in bytes
        H5P_prop_within_t type; IN: Type of object the property will be inserted into
        void *value;            IN: Pointer to buffer containing value for property
        H5P_prp_create_func_t prp_create;   IN: Function pointer to property
                                    creation callback
        H5P_prp_set_func_t prp_set; IN: Function pointer to property set callback
        H5P_prp_get_func_t prp_get; IN: Function pointer to property get callback
        H5P_prp_delete_func_t prp_delete; IN: Function pointer to property delete callback
        H5P_prp_copy_func_t prp_copy; IN: Function pointer to property copy callback
        H5P_prp_compare_func_t prp_cmp; IN: Function pointer to property compare callback
        H5P_prp_close_func_t prp_close; IN: Function pointer to property close
                                    callback
 RETURNS
    Returns a pointer to the newly created property on success,
        NULL on failure.
 DESCRIPTION
    Allocates memory and copies property information into a new property object.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genprop_t *
H5P_create_prop(const char *name, size_t size, H5P_prop_within_t type,
    void *value,
    H5P_prp_create_func_t prp_create, H5P_prp_set_func_t prp_set,
    H5P_prp_get_func_t prp_get, H5P_prp_delete_func_t prp_delete,
    H5P_prp_copy_func_t prp_copy, H5P_prp_compare_func_t prp_cmp,
    H5P_prp_close_func_t prp_close)
{
    H5P_genprop_t *prop=NULL;        /* Pointer to new property copied */
    H5P_genprop_t *ret_value;        /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_create_prop);

    assert(name);
    assert((size>0 && value!=NULL) || (size==0));
    assert(type!=H5P_PROP_WITHIN_UNKNOWN);

    /* Allocate the new property */
    if (NULL==(prop = H5FL_MALLOC (H5P_genprop_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Set the property initial values */
    prop->name = H5MM_xstrdup(name); /* Duplicate name */
    prop->shared_name=0;
    prop->size=size;
    prop->type=type;

    /* Duplicate value, if it exists */
    if(value!=NULL) {
        if (NULL==(prop->value = H5MM_malloc (prop->size)))
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
        HDmemcpy(prop->value,value,prop->size);
    } /* end if */
    else
        prop->value=NULL;

    /* Set the function pointers */
    prop->create=prp_create;
    prop->set=prp_set;
    prop->get=prp_get;
    prop->del=prp_delete;
    prop->copy=prp_copy;
    /* Use custom comparison routine if available, otherwise default to memcmp() */
    if(prp_cmp!=NULL)
        prop->cmp=prp_cmp;
    else
        prop->cmp=&memcmp;
    prop->close=prp_close;

    /* Set return value */
    ret_value=prop;

done:
    /* Free any resources allocated */
    if(ret_value==NULL) {
        if(prop!=NULL) {
            if(prop->name!=NULL)
                H5MM_xfree(prop->name);
            if(prop->value!=NULL)
                H5MM_xfree(prop->value);
            H5FL_FREE(H5P_genprop_t,prop);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_create_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_add_prop
 PURPOSE
    Internal routine to insert a property into a property skip list
 USAGE
    herr_t H5P_add_prop(slist, prop)
        H5SL_t *slist;          IN/OUT: Pointer to skip list of properties
        H5P_genprop_t *prop;    IN: Pointer to property to insert
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Inserts a property into a skip list of properties.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_add_prop(H5SL_t *slist, H5P_genprop_t *prop)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5P_add_prop,FAIL);

    assert(slist);
    assert(prop);
    assert(prop->type!=H5P_PROP_WITHIN_UNKNOWN);

    /* Insert property into skip list */
    if(H5SL_insert(slist,prop,prop->name)<0)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into skip list");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_add_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_find_prop_plist
 PURPOSE
    Internal routine to check for a property in a property list's skip list
 USAGE
    H5P_genprop_t *H5P_find_prop(plist, name)
        H5P_genplist_t *plist;  IN: Pointer to property list to check
        const char *name;       IN: Name of property to check for
 RETURNS
    Returns pointer to property on success, NULL on failure.
 DESCRIPTION
    Checks for a property in a property list's skip list of properties.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genprop_t *
H5P_find_prop_plist(H5P_genplist_t *plist, const char *name)
{
    H5P_genprop_t *ret_value;   /* Property pointer return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_find_prop_plist);

    assert(plist);
    assert(name);

    /* Check if the property has been deleted from list */
    if(H5SL_search(plist->del,name)!=NULL) {
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,NULL,"can't find property in skip list");
    } /* end if */
    else {
        /* Get the property data from the skip list */
        if((ret_value=H5SL_search(plist->props,name))==NULL) {
            H5P_genclass_t *tclass;     /* Temporary class pointer */

            /* Couldn't find property in list itself, start searching through class info */
            tclass=plist->pclass;
            while(tclass!=NULL) {
                /* Find the property in the class */
                if((ret_value=H5SL_search(tclass->props,name))!=NULL)
                    /* Got pointer to property - leave now */
                    break;

                /* Go up to parent class */
                tclass=tclass->parent;
            } /* end while */

            /* Check if we haven't found the property */
            if(ret_value==NULL)
                HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,NULL,"can't find property in skip list");
        } /* end else */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_find_prop_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_find_prop_pclass
 PURPOSE
    Internal routine to check for a property in a class skip list
 USAGE
    H5P_genprop_t *H5P_find_prop_class(pclass, name)
        H5P_genclass *pclass;   IN: Pointer generic property class to check
        const char *name;       IN: Name of property to check for
 RETURNS
    Returns pointer to property on success, NULL on failure.
 DESCRIPTION
    Checks for a property in a class's skip list of properties.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genprop_t *
H5P_find_prop_pclass(H5P_genclass_t *pclass, const char *name)
{
    H5P_genprop_t *ret_value;   /* Property pointer return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_find_prop_pclass);

    assert(pclass);
    assert(name);

    /* Get the property from the skip list */
    if((ret_value=H5SL_search(pclass->props,name))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,NULL,"can't find property in skip list");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_find_prop_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5P_free_prop
 PURPOSE
    Internal routine to destroy a property node
 USAGE
    herr_t H5P_free_prop(prop)
        H5P_genprop_t *prop;    IN: Pointer to property to destroy
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Releases all the memory for a property list.  Does _not_ call the
    properties 'close' callback, that should already have been done.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_free_prop(H5P_genprop_t *prop)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_free_prop);

    assert(prop);

    /* Release the property value if it exists */
    if(prop->value)
        H5MM_xfree(prop->value);

    /* Only free the name if we own it */
    if(prop->shared_name==0)
        H5MM_xfree(prop->name);

    H5FL_FREE(H5P_genprop_t,prop);

    FUNC_LEAVE_NOAPI(SUCCEED);
}   /* H5P_free_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_free_prop_cb
 PURPOSE
    Internal routine to properties from a property skip list
 USAGE
    herr_t H5P_free_prop_cb(item, key, op_data)
        void *item;             IN/OUT: Pointer to property
        void *key;              IN/OUT: Pointer to property key
        void *_make_cb;         IN: Whether to make property callbacks or not
 RETURNS
    Returns zero on success, negative on failure.
 DESCRIPTION
        Calls the property 'close' callback for a property & frees property
    info.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_free_prop_cb(void *item, void UNUSED *key, void *op_data)
{
    H5P_genprop_t *tprop=(H5P_genprop_t *)item;       /* Temporary pointer to property */
    unsigned make_cb=*(unsigned *)op_data;     /* Whether to make property 'close' callback */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_free_prop_cb);

    assert(tprop);

    /* Call the close callback and ignore the return value, there's nothing we can do about it */
    if(make_cb && tprop->close!=NULL)
        (tprop->close)(tprop->name,tprop->size,tprop->value);

    /* Free the property, ignoring return value, nothing we can do */
    H5P_free_prop(tprop);

    FUNC_LEAVE_NOAPI(0);
}   /* H5P_free_prop_cb() */


/*--------------------------------------------------------------------------
 NAME
    H5P_free_del_name_cb
 PURPOSE
    Internal routine to free 'deleted' property name
 USAGE
    herr_t H5P_free_del_name_cb(item, key, op_data)
        void *item;             IN/OUT: Pointer to deleted name
        void *key;              IN/OUT: Pointer to key
        void *op_data;          IN: Operator callback data (unused)
 RETURNS
    Returns zero on success, negative on failure.
 DESCRIPTION
    Frees the deleted property name
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_free_del_name_cb(void *item, void UNUSED *key, void UNUSED *op_data)
{
    char *del_name=(char *)item;       /* Temporary pointer to deleted name */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_free_del_name_cb);

    assert(del_name);

    /* Free the name */
    H5MM_xfree(del_name);

    FUNC_LEAVE_NOAPI(0);
}   /* H5P_free_del_name_cb() */


/*--------------------------------------------------------------------------
 NAME
    H5P_access_class
 PURPOSE
    Internal routine to increment or decrement list & class dependancies on a
        property list class
 USAGE
    herr_t H5P_access_class(pclass,mod)
        H5P_genclass_t *pclass;     IN: Pointer to class to modify
        H5P_class_mod_t mod;        IN: Type of modification to class
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Increment/Decrement the class or list dependancies for a given class.
    This routine is the final arbiter on decisions about actually releasing a
    class in memory, such action is only taken when the reference counts for
    both dependent classes & lists reach zero.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_access_class(H5P_genclass_t *pclass, H5P_class_mod_t mod)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_access_class);

    assert(pclass);
    assert(mod>H5P_MOD_ERR && mod<H5P_MOD_MAX);

    switch(mod) {
        case H5P_MOD_INC_CLS:        /* Increment the dependant class count*/
            pclass->classes++;
            break;

        case H5P_MOD_DEC_CLS:        /* Decrement the dependant class count*/
            pclass->classes--;
            break;

        case H5P_MOD_INC_LST:        /* Increment the dependant list count*/
            pclass->plists++;
            break;

        case H5P_MOD_DEC_LST:        /* Decrement the dependant list count*/
            pclass->plists--;
            break;

        case H5P_MOD_INC_REF:        /* Increment the ID reference count*/
            /* Reset the deleted flag if incrementing the reference count */
            if(pclass->deleted)
                pclass->deleted=0;
            pclass->ref_count++;
            break;

        case H5P_MOD_DEC_REF:        /* Decrement the ID reference count*/
            pclass->ref_count--;

            /* Mark the class object as deleted if reference count drops to zero */
            if(pclass->ref_count==0)
                pclass->deleted=1;
            break;

        case H5P_MOD_ERR:
        case H5P_MOD_MAX:
            assert(0 && "Invalid H5P class modification");
    } /* end switch */

    /* Check if we can release the class information now */
    if(pclass->deleted && pclass->plists==0 && pclass->classes==0 ) {
        H5P_genclass_t *par_class=pclass->parent;       /* Pointer to class's parent */

        assert(pclass->name);
        H5MM_xfree(pclass->name);

        /* Free the class properties without making callbacks */
        if(pclass->props) {
            unsigned make_cb=0;

            H5SL_destroy(pclass->props,H5P_free_prop_cb,&make_cb);
        } /* end if */

        H5FL_FREE(H5P_genclass_t,pclass);

        /* Reduce the number of dependent classes on parent class also */
        if(par_class!=NULL)
            H5P_access_class(par_class, H5P_MOD_DEC_CLS);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED);
}   /* H5P_access_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_check_class
 PURPOSE
    Internal callback routine to check for duplicated names in parent class.
 USAGE
    int H5P_check_class(obj, id, key)
        H5P_genclass_t *obj;    IN: Pointer to class
        hid_t id;               IN: ID of object being looked at
        const void *key;        IN: Pointer to information used to compare
                                    classes.
 RETURNS
    Returns >0 on match, 0 on no match and <0 on failure.
 DESCRIPTION
    Checks whether a property list class has the same parent and name as a
    new class being created.  This is a callback routine for H5I_search()
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5P_check_class(void *_obj, hid_t UNUSED id, void *_key)
{
    H5P_genclass_t *obj=(H5P_genclass_t *)_obj; /* Pointer to the class for this ID */
    const H5P_check_class_t *key=(const H5P_check_class_t *)_key; /* Pointer to key information for comparison */
    int ret_value=0;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_check_class);

    assert(obj);
    assert(H5I_GENPROP_CLS==H5I_get_type(id));
    assert(key);

    /* Check if the class object has the same parent as the new class */
    if(obj->parent==key->parent) {
        /* Check if they have the same name */
        if(HDstrcmp(obj->name,key->name)==0)
            ret_value=1;        /* Indicate a match */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5P_check_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_create_class
 PURPOSE
    Internal routine to create a new property list class.
 USAGE
    H5P_genclass_t H5P_create_class(par_class, name, internal,
                cls_create, create_data, cls_close, close_data)
        H5P_genclass_t *par_class;  IN: Pointer to parent class
        const char *name;       IN: Name of class we are creating
        unsigned internal; IN: Whether this is an internal class or not
        H5P_cls_create_func_t;  IN: The callback function to call when each
                                    property list in this class is created.
        void *create_data;      IN: Pointer to user data to pass along to class
                                    creation callback.
        H5P_cls_copy_func_t;    IN: The callback function to call when each
                                    property list in this class is copied.
        void *copy_data;        IN: Pointer to user data to pass along to class
                                    copy callback.
        H5P_cls_close_func_t;   IN: The callback function to call when each
                                    property list in this class is closed.
        void *close_data;       IN: Pointer to user data to pass along to class
                                    close callback.
 RETURNS
    Returns a pointer to the newly created property list class on success,
        NULL on failure.
 DESCRIPTION
    Allocates memory and attaches a class to the property list class hierarchy.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genclass_t *
H5P_create_class(H5P_genclass_t *par_class, const char *name, unsigned internal,
    H5P_cls_create_func_t cls_create, void *create_data,
    H5P_cls_copy_func_t cls_copy, void *copy_data,
    H5P_cls_close_func_t cls_close, void *close_data
    )
{
    H5P_genclass_t *pclass=NULL;   /* Property list class created */
    H5P_genclass_t *ret_value;     /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_create_class);

    assert(name);
    /* Allow internal classes to break some rules */
    /* (This allows the root of the tree to be created with this routine -QAK) */
    if(!internal) {
        assert(par_class);
    }

    /* Allocate room for the class */
    if (NULL==(pclass = H5FL_CALLOC(H5P_genclass_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,"memory allocation failed");

    /* Set class state */
    pclass->parent = par_class;
    pclass->name = H5MM_xstrdup(name);
    pclass->nprops = 0;     /* Classes are created without properties initially */
    pclass->plists = 0;     /* No properties lists of this class yet */
    pclass->classes = 0;    /* No classes derived from this class yet */
    pclass->ref_count = 1;  /* This is the first reference to the new class */
    pclass->internal = internal;
    pclass->deleted = 0;    /* Not deleted yet... :-) */
    pclass->revision = H5P_GET_NEXT_REV;        /* Get a revision number for the class */

    /* Create the skip list for properties */
    if((pclass->props=H5SL_create(H5SL_TYPE_STR,0.5,H5P_DEFAULT_SKIPLIST_HEIGHT))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,NULL,"can't create skip list for properties");

    /* Set callback functions and pass-along data */
    pclass->create_func = cls_create;
    pclass->create_data = create_data;
    pclass->copy_func = cls_copy;
    pclass->copy_data = copy_data;
    pclass->close_func = cls_close;
    pclass->close_data = close_data;

    /* Increment parent class's derived class value */
    if(par_class!=NULL) {
        if(H5P_access_class(par_class,H5P_MOD_INC_CLS)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, NULL,"Can't increment parent class ref count");
    } /* end if */

    /* Set return value */
    ret_value=pclass;

done:
    /* Free any resources allocated */
    if(ret_value==NULL) {
        if(pclass!=NULL)
            H5FL_FREE(H5P_genclass_t,pclass);
    }

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_create_class() */


/*--------------------------------------------------------------------------
 NAME
    H5Pcreate_class
 PURPOSE
    Create a new property list class.
 USAGE
    hid_t H5Pcreate_class(parent, name, cls_create, create_data,
                cls_close, close_data)
        hid_t parent;       IN: Property list class ID of parent class
        const char *name;   IN: Name of class we are creating
        H5P_cls_create_func_t cls_create;   IN: The callback function to call
                                    when each property list in this class is
                                    created.
        void *create_data;  IN: Pointer to user data to pass along to class
                                    creation callback.
        H5P_cls_copy_func_t cls_copy;   IN: The callback function to call
                                    when each property list in this class is
                                    copied.
        void *copy_data;  IN: Pointer to user data to pass along to class
                                    copy callback.
        H5P_cls_close_func_t cls_close;     IN: The callback function to call
                                    when each property list in this class is
                                    closed.
        void *close_data;   IN: Pointer to user data to pass along to class
                                    close callback.
 RETURNS
    Returns a valid property list class ID on success, NULL on failure.
 DESCRIPTION
    Allocates memory and attaches a class to the property list class hierarchy.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5Pcreate_class(hid_t parent, const char *name,
    H5P_cls_create_func_t cls_create, void *create_data,
    H5P_cls_copy_func_t cls_copy, void *copy_data,
    H5P_cls_close_func_t cls_close, void *close_data
    )
{
    H5P_genclass_t  *par_class = NULL;  /* Pointer to the parent class */
    H5P_genclass_t  *pclass = NULL;     /* Property list class created */
    hid_t  ret_value;                  /* Return value       */

    FUNC_ENTER_API(H5Pcreate_class, FAIL);
    H5TRACE8("i","isxxxxxx",parent,name,cls_create,create_data,cls_copy,
             copy_data,cls_close,close_data);

    /* Check arguments. */
    if (H5P_DEFAULT!=parent && (H5I_GENPROP_CLS!=H5I_get_type(parent)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class");
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid class name");
    if ((create_data!=NULL && cls_create==NULL)
            || (copy_data!=NULL && cls_copy==NULL)
            || (close_data!=NULL && cls_close==NULL))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "data specified, but no callback provided");

    /* Get the pointer to the parent class */
    if(parent==H5P_DEFAULT)
        par_class=NULL;
    else if (NULL == (par_class = H5I_object(parent)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't retrieve parent class");

    /* Create the new property list class */
    if (NULL==(pclass=H5P_create_class(par_class, name, 0, cls_create, create_data, cls_copy, copy_data, cls_close, close_data)))
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, FAIL, "unable to create property list class");

    /* Get an atom for the class */
    if ((ret_value = H5I_register(H5I_GENPROP_CLS, pclass))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to atomize property list class");

done:
    if (ret_value<0 && pclass)
        H5P_close_class(pclass);

    FUNC_LEAVE_API(ret_value);
}   /* H5Pcreate_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_create
 PURPOSE
    Internal routine to create a new property list of a property list class.
 USAGE
    H5P_genplist_t *H5P_create(class)
        H5P_genclass_t *class;  IN: Property list class create list from
 RETURNS
    Returns a pointer to the newly created property list on success,
        NULL on failure.
 DESCRIPTION
        Creates a property list of a given class.  If a 'create' callback
    exists for the property list class, it is called before the
    property list is passed back to the user.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        If this routine is called from a library routine other than
    H5P_c, the calling routine is responsible for getting an ID for
    the property list and calling the class 'create' callback (if one exists)
    and also setting the "class_init" flag.
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genplist_t *
H5P_create(H5P_genclass_t *pclass)
{
    H5P_genclass_t *tclass;         /* Temporary class pointer */
    H5P_genplist_t *plist=NULL;     /* New property list created */
    H5P_genprop_t *tmp;             /* Temporary pointer to parent class properties */
    H5SL_t *seen=NULL;              /* Skip list to hold names of properties already seen */
    H5P_genplist_t *ret_value;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_create);

    assert(pclass);

    /*
     * Create new property list object
     */

    /* Allocate room for the property list */
    if (NULL==(plist = H5FL_CALLOC(H5P_genplist_t)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,"memory allocation failed");

    /* Set class state */
    plist->pclass = pclass;
    plist->nprops = 0;      /* Initially the plist has the same number of properties as the class */
    plist->class_init = 0;  /* Initially, wait until the class callback finishes to set */

    /* Create the skip list for changed properties */
    if((plist->props=H5SL_create(H5SL_TYPE_STR,0.5,H5P_DEFAULT_SKIPLIST_HEIGHT))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,NULL,"can't create skip list for changed properties");

    /* Create the skip list for deleted properties */
    if((plist->del=H5SL_create(H5SL_TYPE_STR,0.5,H5P_DEFAULT_SKIPLIST_HEIGHT))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,NULL,"can't create skip list for deleted properties");

    /* Create the skip list to hold names of properties already seen
     * (This prevents a property in the class hierarchy from having it's
     * 'create' callback called, if a property in the class hierarchy has
     * already been seen)
     */
    if((seen=H5SL_create(H5SL_TYPE_STR,0.5,H5P_DEFAULT_SKIPLIST_HEIGHT))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,NULL,"can't create skip list for seen properties");

    /*
     * Check if we should copy class properties (up through list of parent classes also),
     * initialize each with default value & make property 'create' callback.
     */
    tclass=pclass;
    while(tclass!=NULL) {
        if(tclass->nprops>0) {
            H5SL_node_t *curr_node;   /* Current node in skip list */

            /* Walk through the properties in the old class */
            curr_node=H5SL_first(tclass->props);
            while(curr_node!=NULL) {
                /* Get pointer to property from node */
                tmp=H5SL_item(curr_node);

                /* Only "create" properties we haven't seen before */
                if(H5SL_search(seen,tmp->name)==NULL) {
                    /* Call property creation callback, if it exists */
                    if(tmp->create) {
                        /* Call the callback & insert changed value into skip list (if necessary) */
                        if(H5P_do_prop_cb1(plist->props,tmp,tmp->create)<0)
                            HGOTO_ERROR (H5E_PLIST, H5E_CANTCOPY, NULL,"Can't create property");
                    } /* end if */

                    /* Add property name to "seen" list */
                    if(H5SL_insert(seen,tmp->name,tmp->name)<0)
                        HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,NULL,"can't insert property into seen skip list");

                    /* Increment the number of properties in list */
                    plist->nprops++;
                } /* end if */

                /* Get the next property node in the skip list */
                curr_node=H5SL_next(curr_node);
            } /* end while */
        } /* end if */

        /* Go up to parent class */
        tclass=tclass->parent;
    } /* end while */

    /* Increment the number of property lists derived from class */
    if(H5P_access_class(plist->pclass,H5P_MOD_INC_LST)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, NULL,"Can't increment class ref count");

    /* Set return value */
    ret_value=plist;

done:
    /* Release the skip list of 'seen' properties */
    if(seen!=NULL)
        H5SL_close(seen);

    /* Release resources allocated on failure */
    if(ret_value==NULL) {
        if(plist!=NULL) {
            /* Close & free any changed properties */
            if(plist->props) {
                unsigned make_cb=1;

                H5SL_destroy(plist->props,H5P_free_prop_cb,&make_cb);
            } /* end if */

            /* Close the deleted property skip list */
            if(plist->del)
                H5SL_close(plist->del);

            /* Release the property list itself */
            H5FL_FREE(H5P_genplist_t,plist);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_create() */


/*--------------------------------------------------------------------------
 NAME
    H5P_create_id
 PURPOSE
    Internal routine to create a new property list of a property list class.
 USAGE
    hid_t H5P_create_id(pclass)
        H5P_genclass_t *pclass;       IN: Property list class create list from
 RETURNS
    Returns a valid property list ID on success, FAIL on failure.
 DESCRIPTION
        Creates a property list of a given class.  If a 'create' callback
    exists for the property list class, it is called before the
    property list is passed back to the user.  If 'create' callbacks exist for
    any individual properties in the property list, they are called before the
    class 'create' callback.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5P_create_id(H5P_genclass_t *pclass)
{
    H5P_genplist_t  *plist=NULL;    /* Property list created */
    hid_t plist_id=FAIL;        /* Property list ID */
    hid_t ret_value;            /* return value */

    FUNC_ENTER_NOAPI(H5P_create_id, FAIL);

    assert(pclass);

    /* Create the new property list */
    if ((plist=H5P_create(pclass))==NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, FAIL, "unable to create property list");

    /* Get an atom for the property list */
    if ((plist_id = H5I_register(H5I_GENPROP_LST, plist))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to atomize property list");

    /* Save the property list ID in the property list struct, for use in the property class's 'close' callback */
    plist->plist_id=plist_id;

    /* Call the class callback (if it exists) now that we have the property list ID */
    if(plist->pclass->create_func!=NULL) {
        if((plist->pclass->create_func)(plist_id,plist->pclass->create_data)<0) {
            /* Delete ID, ignore return value */
            H5I_remove(plist_id);
            HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL,"Can't initialize property");
        } /* end if */
    } /* end if */

    /* Set the class initialization flag */
    plist->class_init=1;

    /* Set the return value */
    ret_value=plist_id;

done:
    if (ret_value<0 && plist)
        H5P_close(plist);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_create_id() */


/*--------------------------------------------------------------------------
 NAME
    H5Pcreate
 PURPOSE
    Routine to create a new property list of a property list class.
 USAGE
    hid_t H5Pcreate(cls_id)
        hid_t cls_id;       IN: Property list class create list from
 RETURNS
    Returns a valid property list ID on success, FAIL on failure.
 DESCRIPTION
        Creates a property list of a given class.  If a 'create' callback
    exists for the property list class, it is called before the
    property list is passed back to the user.  If 'create' callbacks exist for
    any individual properties in the property list, they are called before the
    class 'create' callback.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5Pcreate(hid_t cls_id)
{
    H5P_genclass_t  *pclass;   /* Property list class to modify */
    hid_t ret_value;               /* return value */

    FUNC_ENTER_API(H5Pcreate, FAIL);
    H5TRACE1("i","i",cls_id);

    /* Check arguments. */
    if (NULL == (pclass = H5I_object_verify(cls_id, H5I_GENPROP_CLS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class");

    /* Create the new property list */
    if((ret_value=H5P_create_id(pclass))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCREATE, FAIL, "unable to create property list");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pcreate() */


/*--------------------------------------------------------------------------
 NAME
    H5P_register
 PURPOSE
    Internal routine to register a new property in a property list class.
 USAGE
    herr_t H5P_register(class, name, size, default, prp_create, prp_set, prp_get, prp_close)
        H5P_genclass_t *class;  IN: Property list class to close
        const char *name;       IN: Name of property to register
        size_t size;            IN: Size of property in bytes
        void *def_value;        IN: Pointer to buffer containing default value
                                    for property in newly created property lists
        H5P_prp_create_func_t prp_create;   IN: Function pointer to property
                                    creation callback
        H5P_prp_set_func_t prp_set; IN: Function pointer to property set callback
        H5P_prp_get_func_t prp_get; IN: Function pointer to property get callback
        H5P_prp_delete_func_t prp_delete; IN: Function pointer to property delete callback
        H5P_prp_copy_func_t prp_copy; IN: Function pointer to property copy callback
        H5P_prp_compare_func_t prp_cmp; IN: Function pointer to property compare callback
        H5P_prp_close_func_t prp_close; IN: Function pointer to property close
                                    callback
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Registers a new property with a property list class.  The property will
    exist in all property list objects of that class after this routine is
    finished.  The name of the property must not already exist.  The default
    property value must be provided and all new property lists created with this
    property will have the property value set to the default provided.  Any of
    the callback routines may be set to NULL if they are not needed.

        Zero-sized properties are allowed and do not store any data in the
    property list.  These may be used as flags to indicate the presence or
    absence of a particular piece of information.  The 'default' pointer for a
    zero-sized property may be set to NULL.  The property 'create' & 'close'
    callbacks are called for zero-sized properties, but the 'set' and 'get'
    callbacks are never called.

        The 'create' callback is called when a new property list with this
    property is being created.  H5P_prp_create_func_t is defined as:
        typedef herr_t (*H5P_prp_create_func_t)(hid_t prop_id, const char *name,
                size_t size, void *initial_value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being created.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *initial_value; IN/OUT: The initial value for the property being created.
                                (The 'default' value passed to H5Pregister)
    The 'create' routine may modify the value to be set and those changes will
    be stored as the initial value of the property.  If the 'create' routine
    returns a negative value, the new property value is not copied into the
    property and the property list creation routine returns an error value.

        The 'set' callback is called before a new value is copied into the
    property.  H5P_prp_set_func_t is defined as:
        typedef herr_t (*H5P_prp_set_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being modified.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *new_value;    IN/OUT: The value being set for the property.
    The 'set' routine may modify the value to be set and those changes will be
    stored as the value of the property.  If the 'set' routine returns a
    negative value, the new property value is not copied into the property and
    the property list set routine returns an error value.

        The 'get' callback is called before a value is retrieved from the
    property.  H5P_prp_get_func_t is defined as:
        typedef herr_t (*H5P_prp_get_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being queried.
        const char *name;   IN: The name of the property being queried.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value being retrieved for the property.
    The 'get' routine may modify the value to be retrieved and those changes
    will be returned to the calling function.  If the 'get' routine returns a
    negative value, the property value is returned and the property list get
    routine returns an error value.

        The 'delete' callback is called when a property is deleted from a
    property list.  H5P_prp_del_func_t is defined as:
        typedef herr_t (*H5P_prp_del_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list the property is deleted from.
        const char *name;   IN: The name of the property being deleted.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value of the property being deleted.
    The 'delete' routine may modify the value passed in, but the value is not
    used by the library when the 'delete' routine returns.  If the
    'delete' routine returns a negative value, the property list deletion
    routine returns an error value but the property is still deleted.

        The 'copy' callback is called when a property list with this
    property is copied.  H5P_prp_copy_func_t is defined as:
        typedef herr_t (*H5P_prp_copy_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being copied.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being copied.
    The 'copy' routine may modify the value to be copied and those changes will be
    stored as the value of the property.  If the 'copy' routine returns a
    negative value, the new property value is not copied into the property and
    the property list copy routine returns an error value.

        The 'compare' callback is called when a property list with this
    property is compared to another property list.  H5P_prp_compare_func_t is
    defined as:
        typedef int (*H5P_prp_compare_func_t)( void *value1, void *value2,
            size_t size);
    where the parameters to the callback function are:
        const void *value1; IN: The value of the first property being compared.
        const void *value2; IN: The value of the second property being compared.
        size_t size;        IN: The size of the property value
    The 'compare' routine may not modify the values to be compared.  The
    'compare' routine should return a positive value if VALUE1 is greater than
    VALUE2, a negative value if VALUE2 is greater than VALUE1 and zero if VALUE1
    and VALUE2 are equal.

        The 'close' callback is called when a property list with this
    property is being destroyed.  H5P_prp_close_func_t is defined as:
        typedef herr_t (*H5P_prp_close_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being closed.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being closed.
    The 'close' routine may modify the value passed in, but the value is not
    used by the library when the 'close' routine returns.  If the
    'close' routine returns a negative value, the property list close
    routine returns an error value but the property list is still closed.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        The 'set' callback function may be useful to range check the value being
    set for the property or may perform some tranformation/translation of the
    value set.  The 'get' callback would then [probably] reverse the
    transformation, etc.  A single 'get' or 'set' callback could handle
    multiple properties by performing different actions based on the property
    name or other properties in the property list.

        I would like to say "the property list is not closed" when a 'close'
    routine fails, but I don't think that's possible due to other properties in
    the list being successfully closed & removed from the property list.  I
    suppose that it would be possible to just remove the properties which have
    successful 'close' callbacks, but I'm not happy with the ramifications
    of a mangled, un-closable property list hanging around...  Any comments? -QAK

 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_register(H5P_genclass_t *pclass, const char *name, size_t size,
    void *def_value, H5P_prp_create_func_t prp_create, H5P_prp_set_func_t prp_set,
    H5P_prp_get_func_t prp_get, H5P_prp_delete_func_t prp_delete,
    H5P_prp_copy_func_t prp_copy, H5P_prp_compare_func_t prp_cmp,
    H5P_prp_close_func_t prp_close)
{
    H5P_genclass_t *new_class; /* New class pointer */
    H5P_genprop_t *new_prop=NULL;   /* Temporary property pointer */
    H5P_genprop_t *pcopy;      /* Property copy */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5P_register, FAIL);

    assert(pclass);
    assert(name);
    assert((size>0 && def_value!=NULL) || (size==0));

    /* Check for duplicate named properties */
    if(H5SL_search(pclass->props,name)!=NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_EXISTS, FAIL, "property already exists");

    /* Check if class needs to be split because property lists or classes have
     *  been created since the last modification was made to the class.
     */
    if(pclass->plists>0 || pclass->classes>0) {
        if((new_class=H5P_create_class(pclass->parent,pclass->name,
                pclass->internal,pclass->create_func,pclass->create_data,
                pclass->copy_func,pclass->copy_data,
                pclass->close_func,pclass->close_data))==NULL)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL, "can't copy class");

        /* Walk through the skip list of the old class and copy properties */
        if(pclass->nprops>0) {
            H5SL_node_t *curr_node;   /* Current node in skip list */

            /* Walk through the properties in the old class */
            curr_node=H5SL_first(pclass->props);
            while(curr_node!=NULL) {
                /* Make a copy of the class's property */
                if((pcopy=H5P_dup_prop(H5SL_item(curr_node),H5P_PROP_WITHIN_CLASS))==NULL)
                    HGOTO_ERROR (H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property");

                /* Insert the initialized property into the property list */
                if(H5P_add_prop(new_class->props,pcopy)<0)
                    HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into class");

                /* Increment property count for class */
                new_class->nprops++;

                /* Get the next property node in the skip list */
                curr_node=H5SL_next(curr_node);
            } /* end while */
        } /* end if */

        /* Use the new class instead of the old one */
        pclass=new_class;
    } /* end if */

    /* Create property object from parameters */
    if((new_prop=H5P_create_prop(name,size,H5P_PROP_WITHIN_CLASS,def_value,prp_create,prp_set,prp_get,prp_delete,prp_copy,prp_cmp,prp_close))==NULL)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTCREATE, FAIL,"Can't create property");

    /* Insert property into property list class */
    if(H5P_add_prop(pclass->props,new_prop)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into class");

    /* Increment property count for class */
    pclass->nprops++;

    /* Update the revision for the class */
    pclass->revision = H5P_GET_NEXT_REV;

done:
    if(ret_value==FAIL) {
        if(new_prop!=NULL) {
            if(new_prop->name!=NULL)
                H5MM_xfree(new_prop->name);
            if(new_prop->value!=NULL)
                H5MM_xfree(new_prop->value);
            H5FL_FREE(H5P_genprop_t,new_prop);
        } /* end if */
    }  /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_register() */


/*--------------------------------------------------------------------------
 NAME
    H5Pregister
 PURPOSE
    Routine to register a new property in a property list class.
 USAGE
    herr_t H5Pregister(class, name, size, default, prp_create, prp_set, prp_get, prp_close)
        hid_t class;            IN: Property list class to close
        const char *name;       IN: Name of property to register
        size_t size;            IN: Size of property in bytes
        void *def_value;        IN: Pointer to buffer containing default value
                                    for property in newly created property lists
        H5P_prp_create_func_t prp_create;   IN: Function pointer to property
                                    creation callback
        H5P_prp_set_func_t prp_set; IN: Function pointer to property set callback
        H5P_prp_get_func_t prp_get; IN: Function pointer to property get callback
        H5P_prp_delete_func_t prp_delete; IN: Function pointer to property delete callback
        H5P_prp_copy_func_t prp_copy; IN: Function pointer to property copy callback
        H5P_prp_close_func_t prp_close; IN: Function pointer to property close
                                    callback
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Registers a new property with a property list class.  The property will
    exist in all property list objects of that class after this routine is
    finished.  The name of the property must not already exist.  The default
    property value must be provided and all new property lists created with this
    property will have the property value set to the default provided.  Any of
    the callback routines may be set to NULL if they are not needed.

        Zero-sized properties are allowed and do not store any data in the
    property list.  These may be used as flags to indicate the presence or
    absence of a particular piece of information.  The 'default' pointer for a
    zero-sized property may be set to NULL.  The property 'create' & 'close'
    callbacks are called for zero-sized properties, but the 'set' and 'get'
    callbacks are never called.

        The 'create' callback is called when a new property list with this
    property is being created.  H5P_prp_create_func_t is defined as:
        typedef herr_t (*H5P_prp_create_func_t)(hid_t prop_id, const char *name,
                size_t size, void *initial_value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being created.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *initial_value; IN/OUT: The initial value for the property being created.
                                (The 'default' value passed to H5Pregister)
    The 'create' routine may modify the value to be set and those changes will
    be stored as the initial value of the property.  If the 'create' routine
    returns a negative value, the new property value is not copied into the
    property and the property list creation routine returns an error value.

        The 'set' callback is called before a new value is copied into the
    property.  H5P_prp_set_func_t is defined as:
        typedef herr_t (*H5P_prp_set_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being modified.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *new_value;    IN/OUT: The value being set for the property.
    The 'set' routine may modify the value to be set and those changes will be
    stored as the value of the property.  If the 'set' routine returns a
    negative value, the new property value is not copied into the property and
    the property list set routine returns an error value.

        The 'get' callback is called before a value is retrieved from the
    property.  H5P_prp_get_func_t is defined as:
        typedef herr_t (*H5P_prp_get_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being queried.
        const char *name;   IN: The name of the property being queried.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value being retrieved for the property.
    The 'get' routine may modify the value to be retrieved and those changes
    will be returned to the calling function.  If the 'get' routine returns a
    negative value, the property value is returned and the property list get
    routine returns an error value.

        The 'delete' callback is called when a property is deleted from a
    property list.  H5P_prp_del_func_t is defined as:
        typedef herr_t (*H5P_prp_del_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list the property is deleted from.
        const char *name;   IN: The name of the property being deleted.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value of the property being deleted.
    The 'delete' routine may modify the value passed in, but the value is not
    used by the library when the 'delete' routine returns.  If the
    'delete' routine returns a negative value, the property list deletion
    routine returns an error value but the property is still deleted.

        The 'copy' callback is called when a property list with this
    property is copied.  H5P_prp_copy_func_t is defined as:
        typedef herr_t (*H5P_prp_copy_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being copied.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being copied.
    The 'copy' routine may modify the value to be copied and those changes will be
    stored as the value of the property.  If the 'copy' routine returns a
    negative value, the new property value is not copied into the property and
    the property list copy routine returns an error value.

        The 'close' callback is called when a property list with this
    property is being destroyed.  H5P_prp_close_func_t is defined as:
        typedef herr_t (*H5P_prp_close_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being closed.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being closed.
    The 'close' routine may modify the value passed in, but the value is not
    used by the library when the 'close' routine returns.  If the
    'close' routine returns a negative value, the property list close
    routine returns an error value but the property list is still closed.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        The 'set' callback function may be useful to range check the value being
    set for the property or may perform some tranformation/translation of the
    value set.  The 'get' callback would then [probably] reverse the
    transformation, etc.  A single 'get' or 'set' callback could handle
    multiple properties by performing different actions based on the property
    name or other properties in the property list.

        I would like to say "the property list is not closed" when a 'close'
    routine fails, but I don't think that's possible due to other properties in
    the list being successfully closed & removed from the property list.  I
    suppose that it would be possible to just remove the properties which have
    successful 'close' callbacks, but I'm not happy with the ramifications
    of a mangled, un-closable property list hanging around...  Any comments? -QAK

 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Pregister(hid_t cls_id, const char *name, size_t size, void *def_value,
    H5P_prp_create_func_t prp_create, H5P_prp_set_func_t prp_set,
    H5P_prp_get_func_t prp_get, H5P_prp_delete_func_t prp_delete,
    H5P_prp_copy_func_t prp_copy, H5P_prp_close_func_t prp_close)
{
    H5P_genclass_t  *pclass;   /* Property list class to modify */
    herr_t ret_value;     /* return value */

    FUNC_ENTER_API(H5Pregister, FAIL);
    H5TRACE10("e","iszxxxxxxx",cls_id,name,size,def_value,prp_create,prp_set,
             prp_get,prp_delete,prp_copy,prp_close);

    /* Check arguments. */
    if (NULL == (pclass = H5I_object_verify(cls_id, H5I_GENPROP_CLS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class");
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid class name");
    if (size>0 && def_value==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "properties >0 size must have default");

    /* Create the new property list class */
    if ((ret_value=H5P_register(pclass,name,size,def_value,prp_create,prp_set,prp_get,prp_delete,prp_copy,NULL,prp_close))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to register property in class");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pregister() */


/*--------------------------------------------------------------------------
 NAME
    H5P_insert
 PURPOSE
    Internal routine to insert a new property in a property list.
 USAGE
    herr_t H5P_insert(plist, name, size, value, prp_set, prp_get, prp_close)
        H5P_genplist_t *plist;  IN: Property list to add property to
        const char *name;       IN: Name of property to add
        size_t size;            IN: Size of property in bytes
        void *value;            IN: Pointer to the value for the property
        H5P_prp_set_func_t prp_set; IN: Function pointer to property set callback
        H5P_prp_get_func_t prp_get; IN: Function pointer to property get callback
        H5P_prp_delete_func_t prp_delete; IN: Function pointer to property delete callback
        H5P_prp_copy_func_t prp_copy; IN: Function pointer to property copy callback
        H5P_prp_compare_func_t prp_cmp; IN: Function pointer to property compare callback
        H5P_prp_close_func_t prp_close; IN: Function pointer to property close
                                    callback
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Inserts a temporary property into a property list.  The property will
    exist only in this property list object.  The name of the property must not
    already exist.  The value must be provided unless the property is zero-
    sized.  Any of the callback routines may be set to NULL if they are not
    needed.

        Zero-sized properties are allowed and do not store any data in the
    property list.  These may be used as flags to indicate the presence or
    absence of a particular piece of information.  The 'value' pointer for a
    zero-sized property may be set to NULL.  The property 'close' callback is
    called for zero-sized properties, but the 'set' and 'get' callbacks are
    never called.

        The 'set' callback is called before a new value is copied into the
    property.  H5P_prp_set_func_t is defined as:
        typedef herr_t (*H5P_prp_set_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being modified.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *new_value;    IN/OUT: The value being set for the property.
    The 'set' routine may modify the value to be set and those changes will be
    stored as the value of the property.  If the 'set' routine returns a
    negative value, the new property value is not copied into the property and
    the property list set routine returns an error value.

        The 'get' callback is called before a value is retrieved from the
    property.  H5P_prp_get_func_t is defined as:
        typedef herr_t (*H5P_prp_get_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being queried.
        const char *name;   IN: The name of the property being queried.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value being retrieved for the property.
    The 'get' routine may modify the value to be retrieved and those changes
    will be returned to the calling function.  If the 'get' routine returns a
    negative value, the property value is returned and the property list get
    routine returns an error value.

        The 'delete' callback is called when a property is deleted from a
    property list.  H5P_prp_del_func_t is defined as:
        typedef herr_t (*H5P_prp_del_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list the property is deleted from.
        const char *name;   IN: The name of the property being deleted.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value of the property being deleted.
    The 'delete' routine may modify the value passed in, but the value is not
    used by the library when the 'delete' routine returns.  If the
    'delete' routine returns a negative value, the property list deletion
    routine returns an error value but the property is still deleted.

        The 'copy' callback is called when a property list with this
    property is copied.  H5P_prp_copy_func_t is defined as:
        typedef herr_t (*H5P_prp_copy_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being copied.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being copied.
    The 'copy' routine may modify the value to be copied and those changes will be
    stored as the value of the property.  If the 'copy' routine returns a
    negative value, the new property value is not copied into the property and
    the property list copy routine returns an error value.

        The 'compare' callback is called when a property list with this
    property is compared to another property list.  H5P_prp_compare_func_t is
    defined as:
        typedef int (*H5P_prp_compare_func_t)( void *value1, void *value2,
            size_t size);
    where the parameters to the callback function are:
        const void *value1; IN: The value of the first property being compared.
        const void *value2; IN: The value of the second property being compared.
        size_t size;        IN: The size of the property value
    The 'compare' routine may not modify the values to be compared.  The
    'compare' routine should return a positive value if VALUE1 is greater than
    VALUE2, a negative value if VALUE2 is greater than VALUE1 and zero if VALUE1
    and VALUE2 are equal.

        The 'close' callback is called when a property list with this
    property is being destroyed.  H5P_prp_close_func_t is defined as:
        typedef herr_t (*H5P_prp_close_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being closed.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being closed.
    The 'close' routine may modify the value passed in, but the value is not
    used by the library when the 'close' routine returns.  If the
    'close' routine returns a negative value, the property list close
    routine returns an error value but the property list is still closed.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        The 'set' callback function may be useful to range check the value being
    set for the property or may perform some tranformation/translation of the
    value set.  The 'get' callback would then [probably] reverse the
    transformation, etc.  A single 'get' or 'set' callback could handle
    multiple properties by performing different actions based on the property
    name or other properties in the property list.

        There is no 'create' callback routine for temporary property list
    objects, the initial value is assumed to have any necessary setup already
    performed on it.

        I would like to say "the property list is not closed" when a 'close'
    routine fails, but I don't think that's possible due to other properties in
    the list being successfully closed & removed from the property list.  I
    suppose that it would be possible to just remove the properties which have
    successful 'close' callbacks, but I'm not happy with the ramifications
    of a mangled, un-closable property list hanging around...  Any comments? -QAK

 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_insert(H5P_genplist_t *plist, const char *name, size_t size,
    void *value, H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get,
    H5P_prp_delete_func_t prp_delete, H5P_prp_copy_func_t prp_copy,
    H5P_prp_compare_func_t prp_cmp, H5P_prp_close_func_t prp_close)
{
    H5P_genprop_t *new_prop=NULL;       /* Temporary property pointer */
    herr_t ret_value=SUCCEED;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_insert);

    assert(plist);
    assert(name);
    assert((size>0 && value!=NULL) || (size==0));

    /* Check for duplicate named properties */
    if(H5SL_search(plist->props,name)!=NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_EXISTS, FAIL, "property already exists");

    /* Check if the property has been deleted */
    if(H5SL_search(plist->del,name)!=NULL) {
        /* Remove the property name from the deleted property skip list */
        if(H5SL_remove(plist->del,name)==NULL)
            HGOTO_ERROR(H5E_PLIST,H5E_CANTDELETE,FAIL,"can't remove property from deleted skip list");

        /* Fall through to add property to list */
    } /* end if */
    else {
        H5P_genclass_t *tclass;     /* Temporary class pointer */

        /* Check if the property is already in the class hierarchy */
        tclass=plist->pclass;
        while(tclass!=NULL) {
            if(tclass->nprops>0) {
                /* Find the property in the class */
                if(H5SL_search(tclass->props,name)!=NULL)
                    HGOTO_ERROR(H5E_PLIST, H5E_EXISTS, FAIL, "property already exists");
            } /* end if */

            /* Go up to parent class */
            tclass=tclass->parent;
        } /* end while */

        /* Fall through to add property to list */
    } /* end else */

    /* Ok to add to property list */

    /* Create property object from parameters */
    if((new_prop=H5P_create_prop(name,size,H5P_PROP_WITHIN_LIST,value,NULL,prp_set,prp_get,prp_delete,prp_copy,prp_cmp,prp_close))==NULL)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTCREATE, FAIL,"Can't create property");

    /* Insert property into property list class */
    if(H5P_add_prop(plist->props,new_prop)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into class");

    /* Increment property count for class */
    plist->nprops++;

done:
    if(ret_value==FAIL) {
        if(new_prop!=NULL) {
            if(new_prop->name!=NULL)
                H5MM_xfree(new_prop->name);
            if(new_prop->value!=NULL)
                H5MM_xfree(new_prop->value);
            H5FL_FREE(H5P_genprop_t,new_prop);
        } /* end if */
    }  /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_insert() */


/*--------------------------------------------------------------------------
 NAME
    H5Pinsert
 PURPOSE
    Routine to insert a new property in a property list.
 USAGE
    herr_t H5Pinsert(plist, name, size, value, prp_set, prp_get, prp_close)
        hid_t plist;            IN: Property list to add property to
        const char *name;       IN: Name of property to add
        size_t size;            IN: Size of property in bytes
        void *value;            IN: Pointer to the value for the property
        H5P_prp_set_func_t prp_set; IN: Function pointer to property set callback
        H5P_prp_get_func_t prp_get; IN: Function pointer to property get callback
        H5P_prp_delete_func_t prp_delete; IN: Function pointer to property delete callback
        H5P_prp_copy_func_t prp_copy; IN: Function pointer to property copy callback
        H5P_prp_close_func_t prp_close; IN: Function pointer to property close
                                    callback
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Inserts a temporary property into a property list.  The property will
    exist only in this property list object.  The name of the property must not
    already exist.  The value must be provided unless the property is zero-
    sized.  Any of the callback routines may be set to NULL if they are not
    needed.

        Zero-sized properties are allowed and do not store any data in the
    property list.  These may be used as flags to indicate the presence or
    absence of a particular piece of information.  The 'value' pointer for a
    zero-sized property may be set to NULL.  The property 'close' callback is
    called for zero-sized properties, but the 'set' and 'get' callbacks are
    never called.

        The 'set' callback is called before a new value is copied into the
    property.  H5P_prp_set_func_t is defined as:
        typedef herr_t (*H5P_prp_set_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being modified.
        const char *name;   IN: The name of the property being modified.
        size_t size;        IN: The size of the property value
        void *new_value;    IN/OUT: The value being set for the property.
    The 'set' routine may modify the value to be set and those changes will be
    stored as the value of the property.  If the 'set' routine returns a
    negative value, the new property value is not copied into the property and
    the property list set routine returns an error value.

        The 'get' callback is called before a value is retrieved from the
    property.  H5P_prp_get_func_t is defined as:
        typedef herr_t (*H5P_prp_get_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list being queried.
        const char *name;   IN: The name of the property being queried.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value being retrieved for the property.
    The 'get' routine may modify the value to be retrieved and those changes
    will be returned to the calling function.  If the 'get' routine returns a
    negative value, the property value is returned and the property list get
    routine returns an error value.

        The 'delete' callback is called when a property is deleted from a
    property list.  H5P_prp_del_func_t is defined as:
        typedef herr_t (*H5P_prp_del_func_t)(hid_t prop_id, const char *name,
            size_t size, void *value);
    where the parameters to the callback function are:
        hid_t prop_id;      IN: The ID of the property list the property is deleted from.
        const char *name;   IN: The name of the property being deleted.
        size_t size;        IN: The size of the property value
        void *value;        IN/OUT: The value of the property being deleted.
    The 'delete' routine may modify the value passed in, but the value is not
    used by the library when the 'delete' routine returns.  If the
    'delete' routine returns a negative value, the property list deletion
    routine returns an error value but the property is still deleted.

        The 'copy' callback is called when a property list with this
    property is copied.  H5P_prp_copy_func_t is defined as:
        typedef herr_t (*H5P_prp_copy_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being copied.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being copied.
    The 'copy' routine may modify the value to be copied and those changes will be
    stored as the value of the property.  If the 'copy' routine returns a
    negative value, the new property value is not copied into the property and
    the property list copy routine returns an error value.

        The 'close' callback is called when a property list with this
    property is being destroyed.  H5P_prp_close_func_t is defined as:
        typedef herr_t (*H5P_prp_close_func_t)(const char *name, size_t size,
            void *value);
    where the parameters to the callback function are:
        const char *name;   IN: The name of the property being closed.
        size_t size;        IN: The size of the property value
        void *value;        IN: The value of the property being closed.
    The 'close' routine may modify the value passed in, but the value is not
    used by the library when the 'close' routine returns.  If the
    'close' routine returns a negative value, the property list close
    routine returns an error value but the property list is still closed.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        The 'set' callback function may be useful to range check the value being
    set for the property or may perform some tranformation/translation of the
    value set.  The 'get' callback would then [probably] reverse the
    transformation, etc.  A single 'get' or 'set' callback could handle
    multiple properties by performing different actions based on the property
    name or other properties in the property list.

        There is no 'create' callback routine for temporary property list
    objects, the initial value is assumed to have any necessary setup already
    performed on it.

        I would like to say "the property list is not closed" when a 'close'
    routine fails, but I don't think that's possible due to other properties in
    the list being successfully closed & removed from the property list.  I
    suppose that it would be possible to just remove the properties which have
    successful 'close' callbacks, but I'm not happy with the ramifications
    of a mangled, un-closable property list hanging around...  Any comments? -QAK

 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Pinsert(hid_t plist_id, const char *name, size_t size, void *value,
    H5P_prp_set_func_t prp_set, H5P_prp_get_func_t prp_get,
    H5P_prp_delete_func_t prp_delete, H5P_prp_copy_func_t prp_copy,
    H5P_prp_close_func_t prp_close)
{
    H5P_genplist_t  *plist;    /* Property list to modify */
    herr_t ret_value;           /* return value */

    FUNC_ENTER_API(H5Pinsert, FAIL);
    H5TRACE9("e","iszxxxxxx",plist_id,name,size,value,prp_set,prp_get,
             prp_delete,prp_copy,prp_close);

    /* Check arguments. */
    if (NULL == (plist = H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid property name");
    if (size>0 && value==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "properties >0 size must have default");

    /* Create the new property list class */
    if ((ret_value=H5P_insert(plist,name,size,value,prp_set,prp_get,prp_delete,prp_copy,NULL,prp_close))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to register property in plist");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pinsert() */


/*--------------------------------------------------------------------------
 NAME
    H5P_set
 PURPOSE
    Internal routine to set a property's value in a property list.
 USAGE
    herr_t H5P_set(plist, name, value)
        H5P_genplist_t *plist;  IN: Property list to find property in
        const char *name;       IN: Name of property to set
        void *value;            IN: Pointer to the value for the property
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Sets a new value for a property in a property list.  The property name
    must exist or this routine will fail.  If there is a 'set' callback routine
    registered for this property, the 'value' will be passed to that routine and
    any changes to the 'value' will be used when setting the property value.
    The information pointed at by the 'value' pointer (possibly modified by the
    'set' callback) is copied into the property list value and may be changed
    by the application making the H5Pset call without affecting the property
    value.

        If the 'set' callback routine returns an error, the property value will
    not be modified.  This routine may not be called for zero-sized properties
    and will return an error in that case.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_set(H5P_genplist_t *plist, const char *name, const void *value)
{
    H5P_genclass_t *tclass;     /* Temporary class pointer */
    H5P_genprop_t *prop;        /* Temporary property pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5P_set, FAIL);

    assert(plist);
    assert(name);
    assert(value);

    /* Check if the property has been deleted */
    if(H5SL_search(plist->del,name)!=NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property doesn't exist");

    /* Find property in changed list */
    if((prop=H5SL_search(plist->props,name))!=NULL) {
        /* Check for property size >0 */
        if(prop->size==0)
            HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "property has zero size");

        /* Make a copy of the value and pass to 'set' callback */
        if(prop->set!=NULL) {
            void *tmp_value;            /* Temporary value for property */

            /* Make a copy of the current value, in case the callback fails */
            if (NULL==(tmp_value=H5MM_malloc(prop->size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed temporary property value");
            HDmemcpy(tmp_value,value,prop->size);

            /* Call user's callback */
            if((*(prop->set))(plist->plist_id,name,prop->size,tmp_value)<0) {
                H5MM_xfree(tmp_value);
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set property value");
            } /* end if */

            /* Copy new [possibly unchanged] value into property value */
            HDmemcpy(prop->value,tmp_value,prop->size);

            /* Free the temporary value buffer */
            H5MM_xfree(tmp_value);
        } /* end if */
        /* No 'set' callback, just copy value */
        else
            HDmemcpy(prop->value,value,prop->size);
    } /* end if */
    else {
        /*
         * Check if we should set class properties (up through list of parent classes also),
         * & make property 'set' callback.
         */
        tclass=plist->pclass;
        while(tclass!=NULL) {
            if(tclass->nprops>0) {
                /* Find the property in the class */
                if((prop=H5SL_search(tclass->props,name))!=NULL) {
                    H5P_genprop_t *pcopy;  /* Copy of property to insert into skip list */

                    /* Check for property size >0 */
                    if(prop->size==0)
                        HGOTO_ERROR(H5E_PLIST,H5E_BADVALUE,FAIL,"property has zero size");

                    /* Make a copy of the value and pass to 'set' callback */
                    if(prop->set!=NULL) {
                        void *tmp_value;            /* Temporary value for property */

                        /* Make a copy of the current value, in case the callback fails */
                        if (NULL==(tmp_value=H5MM_malloc(prop->size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed temporary property value");
                        HDmemcpy(tmp_value,value,prop->size);

                        /* Call user's callback */
                        if((*(prop->set))(plist->plist_id,name,prop->size,tmp_value)<0) {
                            H5MM_xfree(tmp_value);
                            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set property value");
                        } /* end if */

                        if(HDmemcmp(tmp_value,prop->value,prop->size)) {
                            /* Make a copy of the class's property */
                            if((pcopy=H5P_dup_prop(prop,H5P_PROP_WITHIN_LIST))==NULL)
                                HGOTO_ERROR(H5E_PLIST,H5E_CANTCOPY,FAIL,"Can't copy property");

                            /* Copy new value into property value */
                            HDmemcpy(pcopy->value,tmp_value,pcopy->size);

                            /* Insert the changed property into the property list */
                            if(H5P_add_prop(plist->props,pcopy)<0)
                                HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert changed property into skip list");
                        } /* end if */

                        /* Free the temporary value buffer */
                        H5MM_xfree(tmp_value);
                    } /* end if */
                    /* No 'set' callback, just copy value */
                    else {
                        if(HDmemcmp(value,prop->value,prop->size)) {
                            /* Make a copy of the class's property */
                            if((pcopy=H5P_dup_prop(prop,H5P_PROP_WITHIN_LIST))==NULL)
                                HGOTO_ERROR(H5E_PLIST,H5E_CANTCOPY,FAIL,"Can't copy property");

                            HDmemcpy(pcopy->value,value,pcopy->size);

                            /* Insert the changed property into the property list */
                            if(H5P_add_prop(plist->props,pcopy)<0)
                                HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert changed property into skip list");
                        } /* end if */
                    } /* end else */

                    /* Leave */
                    HGOTO_DONE(SUCCEED);
                } /* end while */
            } /* end if */

            /* Go up to parent class */
            tclass=tclass->parent;
        } /* end while */

        /* If we get this far, then it wasn't in the list of changed properties,
         * nor in the properties in the class hierarchy, indicate an error
         */
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,FAIL,"can't find property in skip list");
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_set() */


/*--------------------------------------------------------------------------
 NAME
    H5Pset
 PURPOSE
    Routine to set a property's value in a property list.
 USAGE
    herr_t H5P_set(plist_id, name, value)
        hid_t plist_id;         IN: Property list to find property in
        const char *name;       IN: Name of property to set
        void *value;            IN: Pointer to the value for the property
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Sets a new value for a property in a property list.  The property name
    must exist or this routine will fail.  If there is a 'set' callback routine
    registered for this property, the 'value' will be passed to that routine and
    any changes to the 'value' will be used when setting the property value.
    The information pointed at by the 'value' pointer (possibly modified by the
    'set' callback) is copied into the property list value and may be changed
    by the application making the H5Pset call without affecting the property
    value.

        If the 'set' callback routine returns an error, the property value will
    not be modified.  This routine may not be called for zero-sized properties
    and will return an error in that case.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Pset(hid_t plist_id, const char *name, void *value)
{
    H5P_genplist_t *plist;      /* Property list to modify */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset, FAIL);
    H5TRACE3("e","isx",plist_id,name,value);

    /* Check arguments. */
    if (NULL == (plist = H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid property name");
    if (value==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalied property value");

    /* Go set the value */
    if(H5P_set(plist,name,value)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to set value in plist");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pset() */


/*--------------------------------------------------------------------------
 NAME
    H5P_exist_plist
 PURPOSE
    Internal routine to query the existance of a property in a property list.
 USAGE
    herr_t H5P_exist_plist(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to check for
 RETURNS
    Success: Positive if the property exists in the property list, zero
            if the property does not exist.
    Failure: negative value
 DESCRIPTION
        This routine checks if a property exists within a property list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5P_exist_plist(H5P_genplist_t *plist, const char *name)
{
    htri_t ret_value=FAIL;     /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_exist_plist);

    assert(plist);
    assert(name);

    /* Check for property in deleted property list */
    if(H5SL_search(plist->del,name)!=NULL)
        ret_value=0;
    else {
        /* Check for property in changed property list */
        if(H5SL_search(plist->props,name)!=NULL)
            ret_value=1;
        else {
            H5P_genclass_t *tclass;     /* Temporary class pointer */

            tclass=plist->pclass;
            while(tclass!=NULL) {
                if(H5SL_search(tclass->props,name)!=NULL)
                    HGOTO_DONE(1);

                /* Go up to parent class */
                tclass=tclass->parent;
            } /* end while */

            /* If we've reached here, we couldn't find the property */
            ret_value=0;
        } /* end else */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_exist_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_exist_pclass
 PURPOSE
    Internal routine to query the existance of a property in a property class.
 USAGE
    herr_t H5P_exist_pclass(pclass, name)
        H5P_genclass_t *pclass;  IN: Property class to check
        const char *name;       IN: Name of property to check for
 RETURNS
    Success: Positive if the property exists in the property list, zero
            if the property does not exist.
    Failure: negative value
 DESCRIPTION
        This routine checks if a property exists within a property list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5P_exist_pclass(H5P_genclass_t *pclass, const char *name)
{
    htri_t ret_value=FAIL;     /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_exist_pclass);

    assert(pclass);
    assert(name);

    /* Check for property in property list */
    if(H5SL_search(pclass->props,name)==NULL)
        ret_value=0;
    else
        ret_value=1;

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_exist_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5Pexist
 PURPOSE
    Routine to query the existance of a property in a property object.
 USAGE
    htri_t H5P_exist(id, name)
        hid_t id;           IN: Property object ID to check
        const char *name;   IN: Name of property to check for
 RETURNS
    Success: Positive if the property exists in the property object, zero
            if the property does not exist.
    Failure: negative value
 DESCRIPTION
        This routine checks if a property exists within a property list or
    class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5Pexist(hid_t id, const char *name)
{
    H5P_genplist_t  *plist;    /* Property list to query */
    H5P_genclass_t  *pclass;   /* Property class to query */
    htri_t ret_value;           /* return value */

    FUNC_ENTER_API(H5Pexist, FAIL);
    H5TRACE2("t","is",id,name);

    /* Check arguments. */
    if (H5I_GENPROP_LST != H5I_get_type(id) && H5I_GENPROP_CLS != H5I_get_type(id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property object");
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid property name");

    /* Check for the existance of the property in the list or class */
    if(H5I_GENPROP_LST == H5I_get_type(id)) {
        if (NULL == (plist = H5I_object(id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");
        if ((ret_value=H5P_exist_plist(plist,name))<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "property does not exist in list");
    } /* end if */
    else
        if(H5I_GENPROP_CLS == H5I_get_type(id)) {
            if (NULL == (pclass = H5I_object(id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property class");
            if ((ret_value=H5P_exist_pclass(pclass,name))<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "property does not exist in class");
        } /* end if */
        else
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property object");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pexist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_size_plist
 PURPOSE
    Internal routine to query the size of a property in a property list.
 USAGE
    herr_t H5P_get_size_plist(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
        size_t *size;           OUT: Size of property
 RETURNS
    Success: non-negative value
    Failure: negative value
 DESCRIPTION
        This routine retrieves the size of a property's value in bytes.  Zero-
    sized properties are allowed and return a value of 0.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_get_size_plist(H5P_genplist_t *plist, const char *name, size_t *size)
{
    H5P_genprop_t *prop;        /* Temporary property pointer */
    herr_t ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_get_size_plist);

    assert(plist);
    assert(name);
    assert(size);

    /* Find property */
    if((prop=H5P_find_prop_plist(plist,name))==NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property doesn't exist");

    /* Get property size */
    *size=prop->size;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_get_size_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_size_pclass
 PURPOSE
    Internal routine to query the size of a property in a property class.
 USAGE
    herr_t H5P_get_size_pclass(pclass, name)
        H5P_genclass_t *pclass; IN: Property class to check
        const char *name;       IN: Name of property to query
        size_t *size;           OUT: Size of property
 RETURNS
    Success: non-negative value
    Failure: negative value
 DESCRIPTION
        This routine retrieves the size of a property's value in bytes.  Zero-
    sized properties are allowed and return a value of 0.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_get_size_pclass(H5P_genclass_t *pclass, const char *name, size_t *size)
{
    H5P_genprop_t *prop;        /* Temporary property pointer */
    herr_t ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_get_size_pclass);

    assert(pclass);
    assert(name);
    assert(size);

    /* Find property */
    if((prop=H5P_find_prop_pclass(pclass,name))==NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property doesn't exist");

    /* Get property size */
    *size=prop->size;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_get_size_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5Pget_size
 PURPOSE
    Routine to query the size of a property in a property list or class.
 USAGE
    herr_t H5Pget_size(id, name)
        hid_t id;               IN: ID of property list or class to check
        const char *name;       IN: Name of property to query
        size_t *size;           OUT: Size of property
 RETURNS
    Success: non-negative value
    Failure: negative value
 DESCRIPTION
        This routine retrieves the size of a property's value in bytes.  Zero-
    sized properties are allowed and return a value of 0.  This function works
    for both property lists and classes.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Pget_size(hid_t id, const char *name, size_t *size)
{
    H5P_genclass_t  *pclass;   /* Property class to query */
    H5P_genplist_t  *plist;    /* Property list to query */
    herr_t ret_value;           /* return value */

    FUNC_ENTER_API(H5Pget_size, FAIL);
    H5TRACE3("e","is*z",id,name,size);

    /* Check arguments. */
    if (H5I_GENPROP_LST != H5I_get_type(id) && H5I_GENPROP_CLS != H5I_get_type(id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property object");
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid property name");
    if (size==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid property size");

    if (H5I_GENPROP_LST == H5I_get_type(id)) {
        if (NULL == (plist = H5I_object(id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");

        /* Check the property size */
        if ((ret_value=H5P_get_size_plist(plist,name,size))<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to query size in plist");
    } /* end if */
    else
        if (H5I_GENPROP_CLS == H5I_get_type(id)) {
            if (NULL == (pclass = H5I_object(id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");

            /* Check the property size */
            if ((ret_value=H5P_get_size_pclass(pclass,name,size))<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to query size in plist");
        } /* end if */
        else
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property object");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pget_size() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_class
 PURPOSE
    Internal routine to query the class of a generic property list
 USAGE
    H5P_genclass_t *H5P_get_class(plist)
        H5P_genplist_t *plist;    IN: Property list to check
 RETURNS
    Success: Pointer to the class for a property list
    Failure: NULL
 DESCRIPTION
    This routine retrieves a pointer to the class for a property list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genclass_t *
H5P_get_class(H5P_genplist_t *plist)
{
    H5P_genclass_t *ret_value;      /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_get_class);

    assert(plist);

    /* Get property size */
    ret_value=plist->pclass;

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_get_class() */


/*--------------------------------------------------------------------------
 NAME
    H5Pget_class
 PURPOSE
    Routine to query the class of a generic property list
 USAGE
    hid_t H5Pget_class(plist_id)
        hid_t plist_id;         IN: Property list to query
 RETURNS
    Success: ID of class object
    Failure: negative
 DESCRIPTION
    This routine retrieves the class of a property list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    Change the name of this function to H5Pget_class (and remove old H5Pget_class)
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5Pget_class(hid_t plist_id)
{
    H5P_genplist_t  *plist;         /* Property list to query */
    H5P_genclass_t  *pclass=NULL;   /* Property list class */
    hid_t ret_value=FAIL;           /* return value */

    FUNC_ENTER_API(H5Pget_class, FAIL);
    H5TRACE1("i","i",plist_id);

    /* Check arguments. */
    if (NULL == (plist = H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");

    /* Retrieve the property list class */
    if ((pclass=H5P_get_class(plist))==NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "unable to query class of property list");

    /* Increment the outstanding references to the class object */
    if(H5P_access_class(pclass,H5P_MOD_INC_REF)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL,"Can't increment class ID ref count");

    /* Get an atom for the class */
    if ((ret_value = H5I_register(H5I_GENPROP_CLS, pclass))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to atomize property list class");

done:
    if (ret_value<0 && pclass)
        H5P_close_class(pclass);

    FUNC_LEAVE_API(ret_value);
}   /* H5Pget_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_nprops_plist
 PURPOSE
    Internal routine to query the number of properties in a property list
 USAGE
    herr_t H5P_get_nprops_plist(plist, nprops)
        H5P_genplist_t *plist;  IN: Property list to check
        size_t *nprops;         OUT: Number of properties in the property list
 RETURNS
    Success: non-negative value
    Failure: negative value
 DESCRIPTION
        This routine retrieves the number of a properties in a property list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_get_nprops_plist(H5P_genplist_t *plist, size_t *nprops)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_get_nprops_plist);

    assert(plist);
    assert(nprops);

    /* Get property size */
    *nprops=plist->nprops;

    FUNC_LEAVE_NOAPI(SUCCEED);
}   /* H5P_get_nprops_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_nprops_pclass
 PURPOSE
    Internal routine to query the number of properties in a property class
 USAGE
    herr_t H5P_get_nprops_pclass(pclass, nprops)
        H5P_genclass_t *pclass;  IN: Property class to check
        size_t *nprops;         OUT: Number of properties in the property list
 RETURNS
    Success: non-negative value (can't fail)
    Failure: negative value
 DESCRIPTION
    This routine retrieves the number of a properties in a property class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_get_nprops_pclass(H5P_genclass_t *pclass, size_t *nprops)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5P_get_nprops_pclass, FAIL)

    assert(pclass);
    assert(nprops);

    /* Get number of properties */
    *nprops=pclass->nprops;

done:
    FUNC_LEAVE_NOAPI(ret_value)
}   /* H5P_get_nprops_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5Pget_nprops
 PURPOSE
    Routine to query the size of a property in a property list or class.
 USAGE
    herr_t H5Pget_nprops(id, nprops)
        hid_t id;               IN: ID of Property list or class to check
        size_t *nprops;         OUT: Number of properties in the property object
 RETURNS
    Success: non-negative value
    Failure: negative value
 DESCRIPTION
        This routine retrieves the number of properties in a property list or
    class.  If a property class ID is given, the number of registered properties
    in the class is returned in NPROPS.  If a property list ID is given, the
    current number of properties in the list is returned in NPROPS.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Pget_nprops(hid_t id, size_t *nprops)
{
    H5P_genplist_t  *plist;    /* Property list to query */
    H5P_genclass_t  *pclass;   /* Property class to query */
    herr_t ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_API(H5Pget_nprops, FAIL);
    H5TRACE2("e","i*z",id,nprops);

    /* Check arguments. */
    if (H5I_GENPROP_LST != H5I_get_type(id) && H5I_GENPROP_CLS != H5I_get_type(id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property object");
    if (nprops==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid property nprops pointer");

    if(H5I_GENPROP_LST == H5I_get_type(id)) {
        if (NULL == (plist = H5I_object(id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");
        if (H5P_get_nprops_plist(plist,nprops)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to query # of properties in plist");
    } /* end if */
    else
        if(H5I_GENPROP_CLS == H5I_get_type(id)) {
            if (NULL == (pclass = H5I_object(id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property class");
            if (H5P_get_nprops_pclass(pclass,nprops)<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to query # of properties in pclass");
        } /* end if */
        else
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property object");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pget_nprops() */


/*--------------------------------------------------------------------------
 NAME
    H5P_cmp_prop
 PURPOSE
    Internal routine to compare two generic properties
 USAGE
    int H5P_cmp_prop(prop1, prop2)
        H5P_genprop_t *prop1;    IN: 1st property to compare
        H5P_genprop_t *prop1;    IN: 2nd property to compare
 RETURNS
    Success: negative if prop1 "less" than prop2, positive if prop1 "greater"
        than prop2, zero if prop1 is "equal" to prop2
    Failure: can't fail
 DESCRIPTION
        This function compares two generic properties together to see if
    they are the same property.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5P_cmp_prop(H5P_genprop_t *prop1, H5P_genprop_t *prop2)
{
    int cmp_value;             /* Value from comparison */
    int ret_value=0;         /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_cmp_prop);

    assert(prop1);
    assert(prop2);

    /* Check the name */
    if((cmp_value=HDstrcmp(prop1->name,prop2->name))!=0)
        HGOTO_DONE(cmp_value);

    /* Check the size of properties */
    if(prop1->size < prop2->size) HGOTO_DONE(-1);
    if(prop1->size > prop2->size) HGOTO_DONE(1);

    /* Check if they both have the same 'create' callback */
    if(prop1->create==NULL && prop2->create!=NULL) HGOTO_DONE(-1);
    if(prop1->create!=NULL && prop2->create==NULL) HGOTO_DONE(1);
    if(prop1->create!=prop2->create) HGOTO_DONE(-1);

    /* Check if they both have the same 'set' callback */
    if(prop1->set==NULL && prop2->set!=NULL) HGOTO_DONE(-1);
    if(prop1->set!=NULL && prop2->set==NULL) HGOTO_DONE(1);
    if(prop1->set!=prop2->set) HGOTO_DONE(-1);

    /* Check if they both have the same 'get' callback */
    if(prop1->get==NULL && prop2->get!=NULL) HGOTO_DONE(-1);
    if(prop1->get!=NULL && prop2->get==NULL) HGOTO_DONE(1);
    if(prop1->get!=prop2->get) HGOTO_DONE(-1);

    /* Check if they both have the same 'delete' callback */
    if(prop1->del==NULL && prop2->del!=NULL) HGOTO_DONE(-1);
    if(prop1->del!=NULL && prop2->del==NULL) HGOTO_DONE(1);
    if(prop1->del!=prop2->del) HGOTO_DONE(-1);

    /* Check if they both have the same 'copy' callback */
    if(prop1->copy==NULL && prop2->copy!=NULL) HGOTO_DONE(-1);
    if(prop1->copy!=NULL && prop2->copy==NULL) HGOTO_DONE(1);
    if(prop1->copy!=prop2->copy) HGOTO_DONE(-1);

    /* Check if they both have the same 'compare' callback */
    if(prop1->cmp==NULL && prop2->cmp!=NULL) HGOTO_DONE(-1);
    if(prop1->cmp!=NULL && prop2->cmp==NULL) HGOTO_DONE(1);
    if(prop1->cmp!=prop2->cmp) HGOTO_DONE(-1);

    /* Check if they both have the same 'close' callback */
    if(prop1->close==NULL && prop2->close!=NULL) HGOTO_DONE(-1);
    if(prop1->close!=NULL && prop2->close==NULL) HGOTO_DONE(1);
    if(prop1->close!=prop2->close) HGOTO_DONE(-1);

    /* Check if they both have values allocated (or not allocated) */
    if(prop1->value==NULL && prop2->value!=NULL) HGOTO_DONE(-1);
    if(prop1->value!=NULL && prop2->value==NULL) HGOTO_DONE(1);
    if(prop1->value!=NULL) {
        /* Call comparison routine */
        if((cmp_value=prop1->cmp(prop1->value,prop2->value,prop1->size))!=0)
            HGOTO_DONE(cmp_value);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_cmp_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_cmp_class
 PURPOSE
    Internal routine to compare two generic property classes
 USAGE
    int H5P_cmp_class(pclass1, pclass2)
        H5P_genclass_t *pclass1;    IN: 1st property class to compare
        H5P_genclass_t *pclass2;    IN: 2nd property class to compare
 RETURNS
    Success: negative if class1 "less" than class2, positive if class1 "greater"
        than class2, zero if class1 is "equal" to class2
    Failure: can't fail
 DESCRIPTION
        This function compares two generic property classes together to see if
    they are the same class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5P_cmp_class(H5P_genclass_t *pclass1, H5P_genclass_t *pclass2)
{
    H5SL_node_t *tnode1,*tnode2;    /* Temporary pointer to property nodes */
    int cmp_value;                  /* Value from comparison */
    int ret_value=0;                /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_cmp_class);

    assert(pclass1);
    assert(pclass2);

    /* Use the revision number to quickly check for identical classes */
    if(pclass1->revision==pclass2->revision)
        HGOTO_DONE(0);

    /* Check the name */
    if((cmp_value=HDstrcmp(pclass1->name,pclass2->name))!=0)
        HGOTO_DONE(cmp_value);

    /* Check the number of properties */
    if(pclass1->nprops < pclass2->nprops) HGOTO_DONE(-1);
    if(pclass1->nprops > pclass2->nprops) HGOTO_DONE(1);

    /* Check the number of property lists created from the class */
    if(pclass1->plists < pclass2->plists) HGOTO_DONE(-1);
    if(pclass1->plists > pclass2->plists) HGOTO_DONE(1);

    /* Check the number of classes derived from the class */
    if(pclass1->classes < pclass2->classes) HGOTO_DONE(-1);
    if(pclass1->classes > pclass2->classes) HGOTO_DONE(1);

    /* Check the number of ID references open on the class */
    if(pclass1->ref_count < pclass2->ref_count) HGOTO_DONE(-1);
    if(pclass1->ref_count > pclass2->ref_count) HGOTO_DONE(1);

    /* Check whether they are internal or not */
    if(pclass1->internal < pclass2->internal) HGOTO_DONE(-1);
    if(pclass1->internal > pclass2->internal) HGOTO_DONE(1);

    /* Check whether they are deleted or not */
    if(pclass1->deleted < pclass2->deleted) HGOTO_DONE(-1);
    if(pclass1->deleted > pclass2->deleted) HGOTO_DONE(1);

    /* Check whether they have creation callback functions & data */
    if(pclass1->create_func==NULL && pclass2->create_func!=NULL) HGOTO_DONE(-1);
    if(pclass1->create_func!=NULL && pclass2->create_func==NULL) HGOTO_DONE(1);
    if(pclass1->create_func!=pclass2->create_func) HGOTO_DONE(-1);
    if(pclass1->create_data < pclass2->create_data) HGOTO_DONE(-1);
    if(pclass1->create_data > pclass2->create_data) HGOTO_DONE(1);

    /* Check whether they have close callback functions & data */
    if(pclass1->close_func==NULL && pclass2->close_func!=NULL) HGOTO_DONE(-1);
    if(pclass1->close_func!=NULL && pclass2->close_func==NULL) HGOTO_DONE(1);
    if(pclass1->close_func!=pclass2->close_func) HGOTO_DONE(-1);
    if(pclass1->close_data < pclass2->close_data) HGOTO_DONE(-1);
    if(pclass1->close_data > pclass2->close_data) HGOTO_DONE(1);

    /* Cycle through the properties and compare them also */
    tnode1=H5SL_first(pclass1->props);
    tnode2=H5SL_first(pclass2->props);
    while(tnode1 || tnode2) {
        H5P_genprop_t *prop1, *prop2;   /* Property for node */

        /* Check if they both have properties in this skip list node */
        if(tnode1==NULL && tnode2!=NULL) HGOTO_DONE(-1);
        if(tnode1!=NULL && tnode2==NULL) HGOTO_DONE(1);

        /* Compare the two properties */
        prop1=H5SL_item(tnode1);
        prop2=H5SL_item(tnode2);
        if((cmp_value=H5P_cmp_prop(prop1,prop2))!=0)
            HGOTO_DONE(cmp_value);

        /* Advance the pointers */
        tnode1=H5SL_next(tnode1);
        tnode2=H5SL_next(tnode2);
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_cmp_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_cmp_plist
 PURPOSE
    Internal routine to compare two generic property lists
 USAGE
    int H5P_cmp_plist(plist1, plist2)
        H5P_genplist_t *plist1;    IN: 1st property list to compare
        H5P_genplist_t *plist2;    IN: 2nd property list to compare
 RETURNS
    Success: negative if list1 "less" than list2, positive if list1 "greater"
        than list2, zero if list1 is "equal" to list2
    Failure: can't fail
 DESCRIPTION
        This function compares two generic property lists together to see if
    they are the same list.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5P_cmp_plist(H5P_genplist_t *plist1, H5P_genplist_t *plist2)
{
    H5SL_node_t *tnode1,*tnode2;    /* Temporary pointer to property nodes */
    int cmp_value;              /* Value from comparison */
    int ret_value=0;            /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_cmp_plist);

    assert(plist1);
    assert(plist2);

    /* Check the number of properties */
    if(plist1->nprops < plist2->nprops) HGOTO_DONE(-1);
    if(plist1->nprops > plist2->nprops) HGOTO_DONE(1);

    /* Check whether they've been initialized */
    if(plist1->class_init < plist2->class_init) HGOTO_DONE(-1);
    if(plist1->class_init > plist2->class_init) HGOTO_DONE(1);

    /* Check for identical deleted properties */
    if(H5SL_count(plist1->del)>0) {
        /* Check for no deleted properties in plist2 */
        if(H5SL_count(plist2->del)==0) HGOTO_DONE(1);

        tnode1=H5SL_first(plist1->del);
        tnode2=H5SL_first(plist2->del);
        while(tnode1 || tnode2) {
            const char *name1, *name2;   /* Name for node */

            /* Check if they both have properties in this node */
            if(tnode1==NULL && tnode2!=NULL) HGOTO_DONE(-1);
            if(tnode1!=NULL && tnode2==NULL) HGOTO_DONE(1);

            /* Compare the two deleted properties */
            name1=H5SL_item(tnode1);
            name2=H5SL_item(tnode2);
            if((cmp_value=HDstrcmp(name1,name2))!=0)
                HGOTO_DONE(cmp_value);

            /* Advance the pointers */
            tnode1=H5SL_next(tnode1);
            tnode2=H5SL_next(tnode2);
        } /* end while */
    } /* end if */
    else
        if(H5SL_count(plist2->del)>0) HGOTO_DONE (-1);

    /* Cycle through the changed properties and compare them also */
    if(H5SL_count(plist1->props)>0) {
        /* Check for no changed properties in plist2 */
        if(H5SL_count(plist2->props)==0) HGOTO_DONE(1);

        tnode1=H5SL_first(plist1->props);
        tnode2=H5SL_first(plist2->props);
        while(tnode1 || tnode2) {
            H5P_genprop_t *prop1, *prop2;   /* Property for node */

            /* Check if they both have properties in this node */
            if(tnode1==NULL && tnode2!=NULL) HGOTO_DONE(-1);
            if(tnode1!=NULL && tnode2==NULL) HGOTO_DONE(1);

            /* Compare the two properties */
            prop1=H5SL_item(tnode1);
            prop2=H5SL_item(tnode2);
            if((cmp_value=H5P_cmp_prop(prop1,prop2))!=0)
                HGOTO_DONE(cmp_value);

            /* Advance the pointers */
            tnode1=H5SL_next(tnode1);
            tnode2=H5SL_next(tnode2);
        } /* end while */
    } /* end if */
    else
        if(H5SL_count(plist2->props)>0) HGOTO_DONE (-1);

    /* Check the parent classes */
    if((cmp_value=H5P_cmp_class(plist1->pclass,plist2->pclass))!=0)
        HGOTO_DONE(cmp_value);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_cmp_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5Pequal
 PURPOSE
    Routine to query whether two property lists or two property classes are equal
 USAGE
    htri_t H5Pequal(id1, id2)
        hid_t id1;         IN: Property list or class ID to compare
        hid_t id2;         IN: Property list or class ID to compare
 RETURNS
    Success: TRUE if equal, FALSE if unequal
    Failure: negative
 DESCRIPTION
    Determines whether two property lists or two property classes are equal.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5Pequal(hid_t id1, hid_t id2)
{
    void *obj1, *obj2;          /* Property objects to compare */
    htri_t ret_value=FALSE;     /* return value */

    FUNC_ENTER_API(H5Pequal, FAIL);
    H5TRACE2("t","ii",id1,id2);

    /* Check arguments. */
    if ((H5I_GENPROP_LST != H5I_get_type(id1) && H5I_GENPROP_CLS != H5I_get_type(id1))
            || (H5I_GENPROP_LST != H5I_get_type(id2) && H5I_GENPROP_CLS != H5I_get_type(id2)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not property objects");
    if (H5I_get_type(id1) != H5I_get_type(id2))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not the same kind of property objects");
    if(NULL == (obj1 = H5I_object(id1)) || NULL == (obj2 = H5I_object(id2)))
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property object doesn't exist");

    /* Compare property lists */
    if(H5I_GENPROP_LST == H5I_get_type(id1)) {
        if(H5P_cmp_plist(obj1,obj2)==0)
            ret_value=TRUE;
    } /* end if */
    /* Must be property classes */
    else {
        if(H5P_cmp_class(obj1,obj2)==0)
            ret_value=TRUE;
    } /* end else */

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pequal() */


/*--------------------------------------------------------------------------
 NAME
    H5P_isa_class_real
 PURPOSE
    Internal routine to query whether a property class is the same as another
    class.
 USAGE
    htri_t H5P_isa_class_real(pclass1, pclass2)
        H5P_genclass_t *pclass1;   IN: Property class to check
        H5P_genclass_t *pclass2;   IN: Property class to compare with
 RETURNS
    Success: TRUE (1) or FALSE (0)
    Failure: negative value
 DESCRIPTION
    This routine queries whether a property class is the same as another class,
    and walks up the hierarchy of derived classes, checking if the first class
    is derived from the second class also.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static htri_t
H5P_isa_class_real(H5P_genclass_t *pclass1, H5P_genclass_t *pclass2)
{
    htri_t ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_isa_class_real);

    assert(pclass1);
    assert(pclass2);

    /* Compare property classes */
    if(H5P_cmp_class(pclass1,pclass2)==0) {
        HGOTO_DONE(TRUE);
    } else {
        /* Check if the class is derived, and walk up the chain, if so */
        if(pclass1->parent!=NULL)
            ret_value=H5P_isa_class_real(pclass1->parent,pclass2);
        else
            HGOTO_DONE(FALSE);
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_isa_class_real() */


/*--------------------------------------------------------------------------
 NAME
    H5P_isa_class
 PURPOSE
    Internal routine to query whether a property list is a certain class
 USAGE
    hid_t H5P_isa_class(plist_id, pclass_id)
        hid_t plist_id;         IN: Property list to query
        hid_t pclass_id;        IN: Property class to query
 RETURNS
    Success: TRUE (1) or FALSE (0)
    Failure: negative
 DESCRIPTION
    This routine queries whether a property list is a member of the property
    list class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This function is special in that it is an internal library function, but
    accepts hid_t's as parameters.  Since it is used in basically the same way
    as the H5I functions, this should be OK.  Don't make more library functions
    which accept hid_t's without thorough discussion. -QAK
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5P_isa_class(hid_t plist_id, hid_t pclass_id)
{
    H5P_genplist_t  *plist;         /* Property list to query */
    H5P_genclass_t  *pclass;        /* Property list class */
    htri_t ret_value;                   /* return value */

    FUNC_ENTER_NOAPI(H5P_isa_class, FAIL);

    /* Check arguments. */
    if (NULL == (plist = H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");
    if (NULL == (pclass = H5I_object_verify(pclass_id, H5I_GENPROP_CLS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property class");

    /* Compare the property list's class against the other class */
    if ((ret_value = H5P_isa_class_real(plist->pclass, pclass))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to compare property list classes");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_isa_class() */


/*--------------------------------------------------------------------------
 NAME
    H5Pisa_class
 PURPOSE
    Routine to query whether a property list is a certain class
 USAGE
    hid_t H5Pisa_class(plist_id, pclass_id)
        hid_t plist_id;         IN: Property list to query
        hid_t pclass_id;        IN: Property class to query
 RETURNS
    Success: TRUE (1) or FALSE (0)
    Failure: negative
 DESCRIPTION
    This routine queries whether a property list is a member of the property
    list class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    What about returning a value indicating that the property class is further
    up the class hierarchy?
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5Pisa_class(hid_t plist_id, hid_t pclass_id)
{
    htri_t ret_value;                   /* return value */

    FUNC_ENTER_API(H5Pisa_class, FAIL);
    H5TRACE2("t","ii",plist_id,pclass_id);

    /* Check arguments. */
    if (H5I_GENPROP_LST != H5I_get_type(plist_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");
    if (H5I_GENPROP_CLS != H5I_get_type(pclass_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property class");

    /* Compare the property list's class against the other class */
    if ((ret_value = H5P_isa_class(plist_id, pclass_id))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to compare property list classes");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pisa_class() */


/*--------------------------------------------------------------------------
 NAME
    H5P_object_verify
 PURPOSE
    Internal routine to query whether a property list is a certain class and
        retrieve the property list object associated with it.
 USAGE
    void *H5P_object_verify(plist_id, pclass_id)
        hid_t plist_id;         IN: Property list to query
        hid_t pclass_id;        IN: Property class to query
 RETURNS
    Success: valid pointer to a property list object
    Failure: NULL
 DESCRIPTION
    This routine queries whether a property list is member of a certain class
    and retrieves the property list object associated with it.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    This function is special in that it is an internal library function, but
    accepts hid_t's as parameters.  Since it is used in basically the same way
    as the H5I functions, this should be OK.  Don't make more library functions
    which accept hid_t's without thorough discussion. -QAK

    This function is similar (in spirit) to H5I_object_verify()
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void *
H5P_object_verify(hid_t plist_id, hid_t pclass_id)
{
    void *ret_value;                   /* return value */

    FUNC_ENTER_NOAPI(H5P_object_verify, NULL);

    /* Compare the property list's class against the other class */
    if (H5P_isa_class(plist_id,pclass_id)!=TRUE)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, NULL, "property list is not a member of the class");

    /* Get the plist structure */
    if(NULL == (ret_value = H5I_object(plist_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, NULL, "can't find object for ID");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_object_verify() */


/*--------------------------------------------------------------------------
 NAME
    H5P_iterate_plist
 PURPOSE
    Internal routine to iterate over the properties in a property list
 USAGE
    herr_t H5P_iterate_plist(plist_id, idx, iter_func, iter_data)
        hid_t plist_id;             IN: ID of property list to iterate over
        int *idx;                   IN/OUT: Index of the property to begin with
        H5P_iterate_t iter_func;    IN: Function pointer to function to be
                                        called with each property iterated over.
        void *iter_data;            IN/OUT: Pointer to iteration data from user
 RETURNS
    Success: Returns the return value of the last call to ITER_FUNC if it was
                non-zero, or zero if all properties have been processed.
    Failure: negative value
 DESCRIPTION
    This routine iterates over the properties in the property object specified
with PLIST_ID.  For each property in the object, the ITER_DATA and some
additional information, specified below, are passed to the ITER_FUNC function.
The iteration begins with the IDX property in the object and the next element
to be processed by the operator is returned in IDX.  If IDX is NULL, then the
iterator starts at the first property; since no stopping point is returned in
this case, the iterator cannot be restarted if one of the calls to its operator
returns non-zero.

The prototype for H5P_iterate_t is:
    typedef herr_t (*H5P_iterate_t)(hid_t id, const char *name, void *iter_data);
The operation receives the property list or class identifier for the object
being iterated over, ID, the name of the current property within the object,
NAME, and the pointer to the operator data passed in to H5Piterate, ITER_DATA.

The return values from an operator are:
    Zero causes the iterator to continue, returning zero when all properties
        have been processed.
    Positive causes the iterator to immediately return that positive value,
        indicating short-circuit success. The iterator can be restarted at the
        index of the next property.
    Negative causes the iterator to immediately return that value, indicating
        failure. The iterator can be restarted at the index of the next
        property.

H5Piterate assumes that the properties in the object identified by ID remains
unchanged through the iteration.  If the membership changes during the
iteration, the function's behavior is undefined.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5P_iterate_plist(hid_t plist_id, int *idx, H5P_iterate_t iter_func, void *iter_data)
{
    H5P_genclass_t *tclass;     /* Temporary class pointer */
    H5P_genplist_t *plist;      /* Property list pointer */
    H5P_genprop_t *tmp;         /* Temporary pointer to properties */
    H5SL_t *seen=NULL;          /* Skip list to hold names of properties already seen */
    H5SL_node_t *curr_node;     /* Current node in skip list */
    int curr_idx=0;             /* Current iteration index */
    int ret_value=FAIL;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_iterate_plist);

    assert(idx);
    assert(iter_func);

    /* Get the property list object */
    if (NULL == (plist = H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");

    /* Create the skip list to hold names of properties already seen */
    if((seen=H5SL_create(H5SL_TYPE_STR,0.5,H5P_DEFAULT_SKIPLIST_HEIGHT))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,FAIL,"can't create skip list for seen properties");

    /* Walk through the changed properties in the list */
    if(H5SL_count(plist->props)>0) {
        curr_node=H5SL_first(plist->props);
        while(curr_node!=NULL) {
            /* Get pointer to property from node */
            tmp=H5SL_item(curr_node);

            /* Check if we've found the correctly indexed property */
            if(curr_idx>=*idx) {
                /* Call the callback function */
                ret_value=(*iter_func)(plist_id,tmp->name,iter_data);

                if(ret_value!=0)
                    HGOTO_DONE(ret_value);
            } /* end if */

            /* Increment the current index */
            curr_idx++;

            /* Add property name to "seen" list */
            if(H5SL_insert(seen,tmp->name,tmp->name)<0)
                HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list");

            /* Get the next property node in the skip list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end if */

    /* Walk up the class hiearchy */
    tclass=plist->pclass;
    while(tclass!=NULL) {
        if(tclass->nprops>0) {
            /* Walk through the properties in the class */
            curr_node=H5SL_first(tclass->props);
            while(curr_node!=NULL) {
                /* Get pointer to property from node */
                tmp=H5SL_item(curr_node);

                /* Only call iterator callback for properties we haven't seen
                 * before and that haven't been deleted
                 */
                if(H5SL_search(seen,tmp->name)==NULL &&
                        H5SL_search(plist->del,tmp->name)==NULL) {


                    /* Check if we've found the correctly indexed property */
                    if(curr_idx>=*idx) {
                        /* Call the callback function */
                        ret_value=(*iter_func)(plist_id,tmp->name,iter_data);

                        if(ret_value!=0)
                            HGOTO_DONE(ret_value);
                    } /* end if */

                    /* Increment the current index */
                    curr_idx++;

                    /* Add property name to "seen" list */
                    if(H5SL_insert(seen,tmp->name,tmp->name)<0)
                        HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list");
                } /* end if */

                /* Get the next property node in the skip list */
                curr_node=H5SL_next(curr_node);
            } /* end while */
        } /* end if */

        /* Go up to parent class */
        tclass=tclass->parent;
    } /* end while */

done:
    /* Set the index we stopped at */
    *idx=curr_idx;

    /* Release the skip list of 'seen' properties */
    if(seen!=NULL)
        H5SL_close(seen);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_iterate_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_iterate_pclass
 PURPOSE
    Internal routine to iterate over the properties in a property class
 USAGE
    herr_t H5P_iterate_pclass(pclass_id, idx, iter_func, iter_data)
        hid_t pclass_id;            IN: ID of property class to iterate over
        int *idx;                   IN/OUT: Index of the property to begin with
        H5P_iterate_t iter_func;    IN: Function pointer to function to be
                                        called with each property iterated over.
        void *iter_data;            IN/OUT: Pointer to iteration data from user
 RETURNS
    Success: Returns the return value of the last call to ITER_FUNC if it was
                non-zero, or zero if all properties have been processed.
    Failure: negative value
 DESCRIPTION
    This routine iterates over the properties in the property object specified
with PCLASS_ID.  For each property in the object, the ITER_DATA and some
additional information, specified below, are passed to the ITER_FUNC function.
The iteration begins with the IDX property in the object and the next element
to be processed by the operator is returned in IDX.  If IDX is NULL, then the
iterator starts at the first property; since no stopping point is returned in
this case, the iterator cannot be restarted if one of the calls to its operator
returns non-zero.

The prototype for H5P_iterate_t is:
    typedef herr_t (*H5P_iterate_t)(hid_t id, const char *name, void *iter_data);
The operation receives the property list or class identifier for the object
being iterated over, ID, the name of the current property within the object,
NAME, and the pointer to the operator data passed in to H5Piterate, ITER_DATA.

The return values from an operator are:
    Zero causes the iterator to continue, returning zero when all properties
        have been processed.
    Positive causes the iterator to immediately return that positive value,
        indicating short-circuit success. The iterator can be restarted at the
        index of the next property.
    Negative causes the iterator to immediately return that value, indicating
        failure. The iterator can be restarted at the index of the next
        property.

H5Piterate assumes that the properties in the object identified by ID remains
unchanged through the iteration.  If the membership changes during the
iteration, the function's behavior is undefined.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5P_iterate_pclass(hid_t pclass_id, int *idx, H5P_iterate_t iter_func, void *iter_data)
{
    H5P_genclass_t *pclass;     /* Property list pointer */
    H5SL_node_t *curr_node;     /* Current node in skip list */
    H5P_genprop_t *prop;        /* Temporary property pointer */
    int curr_idx=0;             /* Current iteration index */
    int ret_value=FAIL;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_iterate_pclass);

    assert(idx);
    assert(iter_func);

    /* Get the property list object */
    if (NULL == (pclass = H5I_object_verify(pclass_id, H5I_GENPROP_CLS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property class");

    /* Cycle through the properties and call the callback */
    curr_idx=0;
    curr_node=H5SL_first(pclass->props);
    while(curr_node!=NULL) {
        if(curr_idx>=*idx) {
            /* Get the property for the node */
            prop=H5SL_item(curr_node);

            /* Call the callback function */
            ret_value=(*iter_func)(pclass_id,prop->name,iter_data);

            /* Check if iteration function succeeded */
            if(ret_value!=0)
                HGOTO_DONE(ret_value);
        } /* end if */

        /* Increment the iteration index */
        curr_idx++;

        /* Get the next property node in the skip list */
        curr_node=H5SL_next(curr_node);
    } /* end while */

done:
    /* Set the index we stopped at */
    *idx=curr_idx;

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_iterate_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5Piterate
 PURPOSE
    Routine to iterate over the properties in a property list or class
 USAGE
    int H5Piterate(pclass_id, idx, iter_func, iter_data)
        hid_t id;                   IN: ID of property object to iterate over
        int *idx;                   IN/OUT: Index of the property to begin with
        H5P_iterate_t iter_func;    IN: Function pointer to function to be
                                        called with each property iterated over.
        void *iter_data;            IN/OUT: Pointer to iteration data from user
 RETURNS
    Success: Returns the return value of the last call to ITER_FUNC if it was
                non-zero, or zero if all properties have been processed.
    Failure: negative value
 DESCRIPTION
    This routine iterates over the properties in the property object specified
with ID.  The properties in both property lists and classes may be iterated
over with this function.  For each property in the object, the ITER_DATA and
some additional information, specified below, are passed to the ITER_FUNC
function.  The iteration begins with the IDX property in the object and the
next element to be processed by the operator is returned in IDX.  If IDX is
NULL, then the iterator starts at the first property; since no stopping point
is returned in this case, the iterator cannot be restarted if one of the calls
to its operator returns non-zero.  The IDX value is 0-based (ie. to start at
the "first" property, the IDX value should be 0).

The prototype for H5P_iterate_t is:
    typedef herr_t (*H5P_iterate_t)(hid_t id, const char *name, void *iter_data);
The operation receives the property list or class identifier for the object
being iterated over, ID, the name of the current property within the object,
NAME, and the pointer to the operator data passed in to H5Piterate, ITER_DATA.

The return values from an operator are:
    Zero causes the iterator to continue, returning zero when all properties
        have been processed.
    Positive causes the iterator to immediately return that positive value,
        indicating short-circuit success. The iterator can be restarted at the
        index of the next property.
    Negative causes the iterator to immediately return that value, indicating
        failure. The iterator can be restarted at the index of the next
        property.

H5Piterate assumes that the properties in the object identified by ID remains
unchanged through the iteration.  If the membership changes during the
iteration, the function's behavior is undefined.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5Piterate(hid_t id, int *idx, H5P_iterate_t iter_func, void *iter_data)
{
    int fake_idx=0;         /* Index when user doesn't provide one */
    int ret_value;          /* return value */

    FUNC_ENTER_API(H5Piterate, FAIL);
    H5TRACE4("Is","i*Isxx",id,idx,iter_func,iter_data);

    /* Check arguments. */
    if (H5I_GENPROP_LST != H5I_get_type(id) && H5I_GENPROP_CLS != H5I_get_type(id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property object");
    if (iter_func==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid iteration callback");

    if (H5I_GENPROP_LST == H5I_get_type(id)) {
        /* Iterate over a property list */
        if ((ret_value=H5P_iterate_plist(id,(idx ? idx : &fake_idx),iter_func,iter_data))<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to iterate over list");
    } /* end if */
    else
        if (H5I_GENPROP_CLS == H5I_get_type(id)) {
            /* Iterate over a property class */
            if ((ret_value=H5P_iterate_pclass(id,(idx ? idx : &fake_idx),iter_func,iter_data))<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to iterate over class");
        } /* end if */
        else
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property object");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Piterate() */


/*--------------------------------------------------------------------------
 NAME
    H5P_peek_unsigned
 PURPOSE
    Internal routine to quickly retrieve the value of a property in a property list.
 USAGE
    int H5P_peek_unsigned(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
 RETURNS
    Directly returns the value of the property in the list
 DESCRIPTION
        This function directly returns the value of a property in a property
    list.  Because this function is only able to just copy a particular property
    value to the return value, there is no way to check for errors.  We attempt
    to make certain that bad things don't happen by validating that the size of
    the property is the same as the size of the return type, but that can't
    catch all errors.
        This function does call the user's 'get' callback routine still.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    No error checking!
    Use with caution!
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
unsigned
H5P_peek_unsigned(H5P_genplist_t *plist, const char *name)
{
    unsigned ret_value;            /* return value */

    FUNC_ENTER_NOAPI(H5P_peek_unsigned, UFAIL);

    assert(plist);
    assert(name);

    /* Get the value to return, don't worry about the return value, we can't return it */
    H5P_get(plist,name,&ret_value);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_peek_unsigned() */


/*--------------------------------------------------------------------------
 NAME
    H5P_peek_hid_t
 PURPOSE
    Internal routine to quickly retrieve the value of a property in a property list.
 USAGE
    hid_t H5P_peek_hid_t(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
 RETURNS
    Directly returns the value of the property in the list
 DESCRIPTION
        This function directly returns the value of a property in a property
    list.  Because this function is only able to just copy a particular property
    value to the return value, there is no way to check for errors.  We attempt
    to make certain that bad things don't happen by validating that the size of
    the property is the same as the size of the return type, but that can't
    catch all errors.
        This function does call the user's 'get' callback routine still.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    No error checking!
    Use with caution!
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5P_peek_hid_t(H5P_genplist_t *plist, const char *name)
{
    hid_t ret_value;            /* return value */

    FUNC_ENTER_NOAPI(H5P_peek_hid_t, FAIL);

    assert(plist);
    assert(name);

    /* Get the value to return, don't worry about the return value, we can't return it */
    H5P_get(plist,name,&ret_value);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_peek_hid_t() */


/*--------------------------------------------------------------------------
 NAME
    H5P_peek_voidp
 PURPOSE
    Internal routine to quickly retrieve the value of a property in a property list.
 USAGE
    void *H5P_peek_voidp(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
 RETURNS
    Directly returns the value of the property in the list
 DESCRIPTION
        This function directly returns the value of a property in a property
    list.  Because this function is only able to just copy a particular property
    value to the return value, there is no way to check for errors.  We attempt
    to make certain that bad things don't happen by validating that the size of
    the property is the same as the size of the return type, but that can't
    catch all errors.
        This function does call the user's 'get' callback routine still.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    No error checking!
    Use with caution!
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
void *
H5P_peek_voidp(H5P_genplist_t *plist, const char *name)
{
    void * ret_value;            /* return value */

    FUNC_ENTER_NOAPI(H5P_peek_voidp, NULL);

    assert(plist);
    assert(name);

    /* Get the value to return, don't worry about the return value, we can't return it */
    H5P_get(plist,name,&ret_value);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_peek_voidp() */


/*--------------------------------------------------------------------------
 NAME
    H5P_peek_size_t
 PURPOSE
    Internal routine to quickly retrieve the value of a property in a property list.
 USAGE
    hsize_t H5P_peek_size_t(plist, name)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
 RETURNS
    Directly returns the value of the property in the list
 DESCRIPTION
        This function directly returns the value of a property in a property
    list.  Because this function is only able to just copy a particular property
    value to the return value, there is no way to check for errors.  We attempt
    to make certain that bad things don't happen by validating that the size of
    the property is the same as the size of the return type, but that can't
    catch all errors.
        This function does call the user's 'get' callback routine still.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    No error checking!
    Use with caution!
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
size_t
H5P_peek_size_t(H5P_genplist_t *plist, const char *name)
{
    size_t ret_value;            /* return value */

    FUNC_ENTER_NOAPI(H5P_peek_size_t, UFAIL);

    assert(plist);
    assert(name);

    /* Get the value to return, don't worry about the return value, we can't return it */
    H5P_get(plist,name,&ret_value);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_peek_size_t() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get
 PURPOSE
    Internal routine to query the value of a property in a property list.
 USAGE
    herr_t H5P_get(plist, name, value)
        H5P_genplist_t *plist;  IN: Property list to check
        const char *name;       IN: Name of property to query
        void *value;            OUT: Pointer to the buffer for the property value
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Retrieves a copy of the value for a property in a property list.  The
    property name must exist or this routine will fail.  If there is a
    'get' callback routine registered for this property, the copy of the
    value of the property will first be passed to that routine and any changes
    to the copy of the value will be used when returning the property value
    from this routine.
        If the 'get' callback routine returns an error, 'value' will not be
    modified and this routine will return an error.  This routine may not be
    called for zero-sized properties.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_get(H5P_genplist_t *plist, const char *name, void *value)
{
    H5P_genclass_t *tclass;     /* Temporary class pointer */
    H5P_genprop_t *prop;        /* Temporary property pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5P_get, FAIL);

    assert(plist);
    assert(name);
    assert(value);

    /* Check if the property has been deleted */
    if(H5SL_search(plist->del,name)!=NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property doesn't exist");

    /* Find property */
    if((prop=H5SL_search(plist->props,name))!=NULL) {
        /* Check for property size >0 */
        if(prop->size==0)
            HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "property has zero size");

        /* Make a copy of the value and pass to 'get' callback */
        if(prop->get!=NULL) {
            void *tmp_value;            /* Temporary value for property */

            /* Make a copy of the current value, in case the callback fails */
            if (NULL==(tmp_value=H5MM_malloc(prop->size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed temporary property value");
            HDmemcpy(tmp_value,prop->value,prop->size);

            /* Call user's callback */
            if((*(prop->get))(plist->plist_id,name,prop->size,tmp_value)<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't get property value");

            /* Copy new [possibly unchanged] value into return value */
            HDmemcpy(value,tmp_value,prop->size);

            /* Free the temporary value buffer */
            H5MM_xfree(tmp_value);
        } /* end if */
        /* No 'get' callback, just copy value */
        else
            HDmemcpy(value,prop->value,prop->size);
    } /* end if */
    else {
        /*
         * Check if we should get class properties (up through list of parent classes also),
         * & make property 'get' callback.
         */
        tclass=plist->pclass;
        while(tclass!=NULL) {
            if(tclass->nprops>0) {
                /* Find the property in the class */
                if((prop=H5SL_search(tclass->props,name))!=NULL) {
                    /* Check for property size >0 */
                    if(prop->size==0)
                        HGOTO_ERROR(H5E_PLIST,H5E_BADVALUE,FAIL,"property has zero size");

                    /* Call the 'get' callback, if there is one */
                    if(prop->get!=NULL) {
                        void *tmp_value;            /* Temporary value for property */

                        /* Make a copy of the current value, in case the callback fails */
                        if (NULL==(tmp_value=H5MM_malloc(prop->size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed temporary property value");
                        HDmemcpy(tmp_value,prop->value,prop->size);

                        /* Call user's callback */
                        if((*(prop->get))(plist->plist_id,name,prop->size,tmp_value)<0) {
                            H5MM_xfree(tmp_value);
                            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't set property value");
                        } /* end if */

                        if(HDmemcmp(tmp_value,prop->value,prop->size)) {
                            H5P_genprop_t *pcopy;  /* Copy of property to insert into skip list */

                            /* Make a copy of the class's property */
                            if((pcopy=H5P_dup_prop(prop,H5P_PROP_WITHIN_LIST))==NULL)
                                HGOTO_ERROR(H5E_PLIST,H5E_CANTCOPY,FAIL,"Can't copy property");

                            /* Copy new value into property value */
                            HDmemcpy(pcopy->value,tmp_value,prop->size);

                            /* Insert the changed property into the property list */
                            if(H5P_add_prop(plist->props,pcopy)<0)
                                HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert changed property into skip list");
                        } /* end if */

                        /* Copy new [possibly unchanged] value into return value */
                        HDmemcpy(value,tmp_value,prop->size);

                        /* Free the temporary value buffer */
                        H5MM_xfree(tmp_value);
                    } /* end if */
                    /* No 'get' callback, just copy value */
                    else
                        HDmemcpy(value,prop->value,prop->size);

                    /* Leave */
                    HGOTO_DONE(SUCCEED);
                } /* end while */
            } /* end if */

            /* Go up to parent class */
            tclass=tclass->parent;
        } /* end while */

        /* If we get this far, then it wasn't in the list of changed properties,
         * nor in the properties in the class hierarchy, indicate an error
         */
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,FAIL,"can't find property in skip list");
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_get() */


/*--------------------------------------------------------------------------
 NAME
    H5Pget
 PURPOSE
    Routine to query the value of a property in a property list.
 USAGE
    herr_t H5Pget(plist_id, name, value)
        hid_t plist_id;         IN: Property list to check
        const char *name;       IN: Name of property to query
        void *value;            OUT: Pointer to the buffer for the property value
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Retrieves a copy of the value for a property in a property list.  The
    property name must exist or this routine will fail.  If there is a
    'get' callback routine registered for this property, the copy of the
    value of the property will first be passed to that routine and any changes
    to the copy of the value will be used when returning the property value
    from this routine.
        If the 'get' callback routine returns an error, 'value' will not be
    modified and this routine will return an error.  This routine may not be
    called for zero-sized properties.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Pget(hid_t plist_id, const char *name, void *value)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget, FAIL);
    H5TRACE3("e","isx",plist_id,name,value);

    /* Check arguments. */
    if(NULL == (plist = H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid property name");
    if (value==NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalied property value");

    /* Go get the value */
    if(H5P_get(plist,name,value)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to query property value");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pget() */


/*--------------------------------------------------------------------------
 NAME
    H5P_remove
 PURPOSE
    Internal routine to remove a property from a property list.
 USAGE
    herr_t H5P_remove(plist, name)
        H5P_genplist_t *plist;  IN: Property list to modify
        const char *name;       IN: Name of property to remove
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Removes a property from a property list.  Both properties which were
    in existance when the property list was created (i.e. properties registered
    with H5Pregister) and properties added to the list after it was created
    (i.e. added with H5Pinsert) may be removed from a property list.
    Properties do not need to be removed a property list before the list itself
    is closed, they will be released automatically when H5Pclose is called.
    The 'close' callback for this property is called before the property is
    release, if the callback exists.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_remove(hid_t plist_id, H5P_genplist_t *plist, const char *name)
{
    H5P_genclass_t *tclass;     /* Temporary class pointer */
    H5P_genprop_t *prop;        /* Temporary property pointer */
    char *del_name;             /* Pointer to deleted name */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5P_remove,FAIL);

    assert(plist);
    assert(name);

    /* Indicate that the property isn't in the list if it has been deleted already */
    if(H5SL_search(plist->del,name)!=NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,FAIL,"can't find property in skip list");

    /* Get the property node from the changed property skip list */
    if((prop=H5SL_search(plist->props,name))!=NULL) {
        /* Pass value to 'close' callback, if it exists */
        if(prop->del!=NULL) {
            /* Call user's callback */
            if((*(prop->del))(plist_id,name,prop->size,prop->value)<0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't close property value");
        } /* end if */

        /* Duplicate string for insertion into new deleted property skip list */
        if((del_name=H5MM_xstrdup(name))==NULL)
            HGOTO_ERROR(H5E_RESOURCE,H5E_NOSPACE,FAIL,"memory allocation failed");

        /* Insert property name into deleted list */
        if(H5SL_insert(plist->del,del_name,del_name)<0)
            HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into deleted skip list");

        /* Remove the property from the skip list */
        if(H5SL_remove(plist->props,prop->name)==NULL)
            HGOTO_ERROR(H5E_PLIST,H5E_CANTDELETE,FAIL,"can't remove property from skip list");

        /* Free the property, ignoring return value, nothing we can do */
        H5P_free_prop(prop);

        /* Decrement the number of properties in list */
        plist->nprops--;
    } /* end if */
    /* Walk through all the properties in the class hierarchy, looking for the property */
    else {
        /*
         * Check if we should delete class properties (up through list of parent classes also),
         * & make property 'delete' callback.
         */
        tclass=plist->pclass;
        while(tclass!=NULL) {
            if(tclass->nprops>0) {
                /* Find the property in the class */
                if((prop=H5P_find_prop_pclass(tclass,name))!=NULL) {
                    /* Pass value to 'del' callback, if it exists */
                    if(prop->del!=NULL) {
                        void *tmp_value;       /* Temporary value buffer */

                        /* Allocate space for a temporary copy of the property value */
                        if (NULL==(tmp_value=H5MM_malloc(prop->size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for temporary property value");
                        HDmemcpy(tmp_value,prop->value,prop->size);

                        /* Call user's callback */
                        if((*(prop->del))(plist_id,name,prop->size,tmp_value)<0) {
                            H5MM_xfree(tmp_value);
                            HGOTO_ERROR(H5E_PLIST, H5E_CANTINIT, FAIL, "can't close property value");
                        } /* end if */

                        /* Release the temporary value buffer */
                        H5MM_xfree(tmp_value);
                    } /* end if */

                    /* Duplicate string for insertion into new deleted property skip list */
                    if((del_name=H5MM_xstrdup(name))==NULL)
                        HGOTO_ERROR(H5E_RESOURCE,H5E_NOSPACE,FAIL,"memory allocation failed");

                    /* Insert property name into deleted list */
                    if(H5SL_insert(plist->del,del_name,del_name)<0)
                        HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into deleted skip list");

                    /* Decrement the number of properties in list */
                    plist->nprops--;

                    /* Leave */
                    HGOTO_DONE(SUCCEED);
                } /* end while */
            } /* end if */

            /* Go up to parent class */
            tclass=tclass->parent;
        } /* end while */

        /* If we get this far, then it wasn't in the list of changed properties,
         * nor in the properties in the class hierarchy, indicate an error
         */
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,FAIL,"can't find property in skip list");
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_remove() */


/*--------------------------------------------------------------------------
 NAME
    H5Premove
 PURPOSE
    Routine to remove a property from a property list.
 USAGE
    herr_t H5Premove(plist_id, name)
        hid_t plist_id;         IN: Property list to modify
        const char *name;       IN: Name of property to remove
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Removes a property from a property list.  Both properties which were
    in existance when the property list was created (i.e. properties registered
    with H5Pregister) and properties added to the list after it was created
    (i.e. added with H5Pinsert) may be removed from a property list.
    Properties do not need to be removed a property list before the list itself
    is closed, they will be released automatically when H5Pclose is called.
    The 'close' callback for this property is called before the property is
    release, if the callback exists.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Premove(hid_t plist_id, const char *name)
{
    H5P_genplist_t  *plist;    /* Property list to modify */
    herr_t ret_value;           /* return value */

    FUNC_ENTER_API(H5Premove, FAIL);
    H5TRACE2("e","is",plist_id,name);

    /* Check arguments. */
    if (NULL == (plist = H5I_object_verify(plist_id, H5I_GENPROP_LST)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid property name");

    /* Create the new property list class */
    if ((ret_value=H5P_remove(plist_id,plist,name))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "unable to remove property");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Premove() */


/*--------------------------------------------------------------------------
 NAME
    H5P_copy_prop_plist
 PURPOSE
    Internal routine to copy a property from one list to another
 USAGE
    herr_t H5P_copy_prop_plist(dst_plist, src_plist, name)
        hid_t dst_id;               IN: ID of destination property list or class
        hid_t src_id;               IN: ID of source property list or class
        const char *name;           IN: Name of property to copy
 RETURNS
    Success: non-negative value.
    Failure: negative value.
 DESCRIPTION
    Copies a property from one property list to another.

    If a property is copied from one list to another, the property will be
    first deleted from the destination list (generating a call to the 'close'
    callback for the property, if one exists) and then the property is copied
    from the source list to the destination list (generating a call to the
    'copy' callback for the property, if one exists).

    If the property does not exist in the destination list, this call is
    equivalent to calling H5Pinsert and the 'create' callback will be called
    (if such a callback exists for the property).

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_copy_prop_plist(hid_t dst_id, hid_t src_id, const char *name)
{
    H5P_genplist_t *dst_plist;      /* Pointer to destination property list */
    H5P_genplist_t *src_plist;      /* Pointer to source property list */
    H5P_genprop_t *prop;            /* Temporary property pointer */
    H5P_genprop_t *new_prop=NULL;   /* Pointer to new property */
    herr_t ret_value=SUCCEED;       /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_copy_prop_plist);

    assert(name);

    /* Get the objects to operate on */
    if(NULL == (src_plist = H5I_object(src_id)) || NULL == (dst_plist = H5I_object(dst_id)))
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property object doesn't exist");

    /* If the property exists in the destination alread */
    if(H5P_find_prop_plist(dst_plist,name)!=NULL) {
        /* Delete the property from the destination list, calling the 'close' callback if necessary */
        if(H5P_remove(dst_id,dst_plist,name)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "unable to remove property");

        /* Get the pointer to the source property */
        prop=H5P_find_prop_plist(src_plist,name);

        /* Make a copy of the source property */
        if((new_prop=H5P_dup_prop(prop,H5P_PROP_WITHIN_LIST))==NULL)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property");

        /* Call property copy callback, if it exists */
        if(new_prop->copy) {
            if((new_prop->copy)(new_prop->name,new_prop->size,new_prop->value)<0)
                HGOTO_ERROR (H5E_PLIST, H5E_CANTCOPY, FAIL,"Can't copy property");
        } /* end if */

        /* Insert the initialized property into the property list */
        if(H5P_add_prop(dst_plist->props,new_prop)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into list");

        /* Increment the number of properties in list */
        dst_plist->nprops++;
    } /* end if */
    /* If not, get the information required to do an H5Pinsert with the property into the destination list */
    else {
        /* Get the pointer to the source property */
        prop=H5P_find_prop_plist(src_plist,name);

        /* Create property object from parameters */
        if((new_prop=H5P_create_prop(prop->name,prop->size,H5P_PROP_WITHIN_LIST,prop->value,prop->create,prop->set,prop->get,prop->del,prop->copy,prop->cmp,prop->close))==NULL)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTCREATE, FAIL,"Can't create property");

        /* Call property creation callback, if it exists */
        if(new_prop->create) {
            if((new_prop->create)(new_prop->name,new_prop->size,new_prop->value)<0)
                HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL,"Can't initialize property");
        } /* end if */

        /* Insert property into property list class */
        if(H5P_add_prop(dst_plist->props,new_prop)<0)
            HGOTO_ERROR (H5E_PLIST, H5E_CANTINSERT, FAIL,"Can't insert property into class");

        /* Increment property count for class */
        dst_plist->nprops++;

    } /* end else */

done:
    /* Cleanup, if necessary */
    if(ret_value<0) {
        if(new_prop!=NULL)
            H5P_free_prop(new_prop);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_copy_prop_plist() */


/*--------------------------------------------------------------------------
 NAME
    H5P_copy_prop_pclass
 PURPOSE
    Internal routine to copy a property from one class to another
 USAGE
    herr_t H5P_copy_prop_pclass(dst_pclass, src_pclass, name)
        H5P_genclass_t  *dst_pclass;    IN: Pointer to destination class
        H5P_genclass_t  *src_pclass;    IN: Pointer to source class
        const char *name;               IN: Name of property to copy
 RETURNS
    Success: non-negative value.
    Failure: negative value.
 DESCRIPTION
    Copies a property from one property class to another.

    If a property is copied from one class to another, all the property
    information will be first deleted from the destination class and then the
    property information will be copied from the source class into the
    destination class.

    If the property does not exist in the destination class or list, this call
    is equivalent to calling H5Pregister.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_copy_prop_pclass(H5P_genclass_t *dst_pclass, H5P_genclass_t *src_pclass, const char *name)
{
    H5P_genprop_t *prop;            /* Temporary property pointer */
    herr_t ret_value=SUCCEED;       /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_copy_prop_pclass);

    assert(dst_pclass);
    assert(src_pclass);
    assert(name);

    /* If the property exists in the destination already */
    if(H5P_exist_pclass(dst_pclass,name)) {
        /* Delete the old property from the destination class */
        if(H5P_unregister(dst_pclass,name)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "unable to remove property");
    } /* end if */

    /* Get the property from the source */
    if((prop=H5P_find_prop_pclass(src_pclass,name))==NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "unable to locate property");

    /* Register the property into the destination */
    if(H5P_register(dst_pclass,name,prop->size,prop->value,prop->create,prop->set,prop->get,prop->del,prop->copy,prop->cmp,prop->close)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTDELETE, FAIL, "unable to remove property");

done:
    /* Cleanup, if necessary */

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_copy_prop_pclass() */


/*--------------------------------------------------------------------------
 NAME
    H5Pcopy_prop
 PURPOSE
    Routine to copy a property from one list or class to another
 USAGE
    herr_t H5Pcopy_prop(dst_id, src_id, name)
        hid_t dst_id;               IN: ID of destination property list or class
        hid_t src_id;               IN: ID of source property list or class
        const char *name;           IN: Name of property to copy
 RETURNS
    Success: non-negative value.
    Failure: negative value.
 DESCRIPTION
    Copies a property from one property list or class to another.

    If a property is copied from one class to another, all the property
    information will be first deleted from the destination class and then the
    property information will be copied from the source class into the
    destination class.

    If a property is copied from one list to another, the property will be
    first deleted from the destination list (generating a call to the 'close'
    callback for the property, if one exists) and then the property is copied
    from the source list to the destination list (generating a call to the
    'copy' callback for the property, if one exists).

    If the property does not exist in the destination class or list, this call
    is equivalent to calling H5Pregister or H5Pinsert (for a class or list, as
    appropriate) and the 'create' callback will be called in the case of the
    property being copied into a list (if such a callback exists for the
    property).

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Pcopy_prop(hid_t dst_id, hid_t src_id, const char *name)
{
    void *src_obj, *dst_obj;    /* Property objects to copy between */
    herr_t ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_API(H5Pcopy_prop, FAIL);
    H5TRACE3("e","iis",dst_id,src_id,name);

    /* Check arguments. */
    if ((H5I_GENPROP_LST != H5I_get_type(src_id) && H5I_GENPROP_CLS != H5I_get_type(src_id))
            || (H5I_GENPROP_LST != H5I_get_type(dst_id) && H5I_GENPROP_CLS != H5I_get_type(dst_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not property objects");
    if (H5I_get_type(src_id) != H5I_get_type(dst_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not the same kind of property objects");
    if (!name || !*name)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no name given");
    if(NULL == (src_obj = H5I_object(src_id)) || NULL == (dst_obj = H5I_object(dst_id)))
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "property object doesn't exist");

    /* Compare property lists */
    if(H5I_GENPROP_LST == H5I_get_type(src_id)) {
        if(H5P_copy_prop_plist(dst_id,src_id,name)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL, "can't copy property between lists");
    } /* end if */
    /* Must be property classes */
    else {
        if(H5P_copy_prop_pclass(dst_obj,src_obj,name)<0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, FAIL, "can't copy property between classes");
    } /* end else */

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pcopy_prop() */


/*--------------------------------------------------------------------------
 NAME
    H5P_unregister
 PURPOSE
    Internal routine to remove a property from a property list class.
 USAGE
    herr_t H5P_unregister(pclass, name)
        H5P_genclass_t *pclass; IN: Property list class to modify
        const char *name;       IN: Name of property to remove
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Removes a property from a property list class.  Future property lists
    created of that class will not contain this property.  Existing property
    lists containing this property are not affected.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static herr_t
H5P_unregister(H5P_genclass_t *pclass, const char *name)
{
    H5P_genprop_t *prop;        /* Temporary property pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_unregister);

    assert(pclass);
    assert(name);

    /* Get the property node from the skip list */
    if((prop=H5SL_search(pclass->props,name))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_NOTFOUND,FAIL,"can't find property in skip list");

    /* Remove the property from the skip list */
    if(H5SL_remove(pclass->props,prop->name)==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTDELETE,FAIL,"can't remove property from skip list");

    /* Free the property, ignoring return value, nothing we can do */
    H5P_free_prop(prop);

    /* Decrement the number of registered properties in class */
    pclass->nprops--;

    /* Update the revision for the class */
    pclass->revision = H5P_GET_NEXT_REV;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_unregister() */


/*--------------------------------------------------------------------------
 NAME
    H5Punregister
 PURPOSE
    Routine to remove a property from a property list class.
 USAGE
    herr_t H5Punregister(pclass_id, name)
        hid_t pclass_id;         IN: Property list class to modify
        const char *name;       IN: Name of property to remove
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Removes a property from a property list class.  Future property lists
    created of that class will not contain this property.  Existing property
    lists containing this property are not affected.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Punregister(hid_t pclass_id, const char *name)
{
    H5P_genclass_t  *pclass;   /* Property list class to modify */
    herr_t ret_value;           /* return value */

    FUNC_ENTER_API(H5Punregister, FAIL);
    H5TRACE2("e","is",pclass_id,name);

    /* Check arguments. */
    if (NULL == (pclass = H5I_object_verify(pclass_id, H5I_GENPROP_CLS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class");
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid property name");

    /* Remove the property list from class */
    if ((ret_value=H5P_unregister(pclass,name))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to remove property from class");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Punregister() */


/*--------------------------------------------------------------------------
 NAME
    H5P_close
 PURPOSE
    Internal routine to close a property list.
 USAGE
    herr_t H5P_close(plist)
        H5P_genplist_t *plist;  IN: Property list to close
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Closes a property list.  If a 'close' callback exists for the property
    list class, it is called before the property list is destroyed.  If 'close'
    callbacks exist for any individual properties in the property list, they are
    called after the class 'close' callback.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
        The property list class 'close' callback routine is not called from
    here, it must have been check for and called properly prior to this routine
    being called
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_close(void *_plist)
{
    H5P_genclass_t *tclass;         /* Temporary class pointer */
    H5P_genplist_t *plist=(H5P_genplist_t *)_plist;
    H5SL_t *seen=NULL;              /* Skip list to hold names of properties already seen */
    size_t nseen;                   /* Number of items 'seen' */
    hbool_t has_parent_class;       /* Flag to indicate that this property list's class has a parent */
    size_t ndel;                    /* Number of items deleted */
    H5SL_node_t *curr_node;         /* Current node in skip list */
    H5P_genprop_t *tmp;             /* Temporary pointer to properties */
    unsigned make_cb=0;             /* Operator data for property free callback */
    herr_t ret_value=SUCCEED;       /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_close);

    assert(plist);

    /* Make call to property list class close callback, if needed */
    if(plist->class_init!=0 && plist->pclass->close_func!=NULL) {
        /* Call user's "close" callback function, ignoring return value */
        (plist->pclass->close_func)(plist->plist_id,plist->pclass->close_data);
    } /* end if */

    /* Create the skip list to hold names of properties already seen
     * (This prevents a property in the class hierarchy from having it's
     * 'close' callback called, if a property in the class hierarchy has
     * already been seen)
     */
    if((seen=H5SL_create(H5SL_TYPE_STR,0.5,H5P_DEFAULT_SKIPLIST_HEIGHT))==NULL)
        HGOTO_ERROR(H5E_PLIST,H5E_CANTCREATE,FAIL,"can't create skip list for seen properties");
    nseen=0;

    /* Walk through the changed properties in the list */
    if(H5SL_count(plist->props)>0) {
        curr_node=H5SL_first(plist->props);
        while(curr_node!=NULL) {
            /* Get pointer to property from node */
            tmp=H5SL_item(curr_node);

            /* Call property close callback, if it exists */
            if(tmp->close) {
                /* Call the 'close' callback */
                (tmp->close)(tmp->name,tmp->size,tmp->value);
            } /* end if */

            /* Add property name to "seen" list */
            if(H5SL_insert(seen,tmp->name,tmp->name)<0)
                HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list");
            nseen++;

            /* Get the next property node in the skip list */
            curr_node=H5SL_next(curr_node);
        } /* end while */
    } /* end if */

    /* Determine number of deleted items from property list */
    ndel=H5SL_count(plist->del);

    /*
     * Check if we should remove class properties (up through list of parent classes also),
     * initialize each with default value & make property 'remove' callback.
     */
    tclass=plist->pclass;
    has_parent_class=(tclass!=NULL && tclass->parent!=NULL && tclass->parent->nprops>0);
    while(tclass!=NULL) {
        if(tclass->nprops>0) {
            /* Walk through the properties in the class */
            curr_node=H5SL_first(tclass->props);
            while(curr_node!=NULL) {
                /* Get pointer to property from node */
                tmp=H5SL_item(curr_node);

                /* Only "delete" properties we haven't seen before
                 * and that haven't already been deleted
                 */
                if((nseen==0 || H5SL_search(seen,tmp->name)==NULL) &&
                        (ndel==0 || H5SL_search(plist->del,tmp->name)==NULL)) {

                    /* Call property close callback, if it exists */
                    if(tmp->close) {
                        void *tmp_value;       /* Temporary value buffer */

                        /* Allocate space for a temporary copy of the property value */
                        if (NULL==(tmp_value=H5MM_malloc(tmp->size)))
                            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for temporary property value");
                        HDmemcpy(tmp_value,tmp->value,tmp->size);

                        /* Call the 'close' callback */
                        (tmp->close)(tmp->name,tmp->size,tmp_value);

                        /* Release the temporary value buffer */
                        H5MM_xfree(tmp_value);
                    } /* end if */

                    /* Add property name to "seen" list, if we have other classes to work on */
                    if(has_parent_class) {
                        if(H5SL_insert(seen,tmp->name,tmp->name)<0)
                            HGOTO_ERROR(H5E_PLIST,H5E_CANTINSERT,FAIL,"can't insert property into seen skip list");
                        nseen++;
                    } /* end if */
                } /* end if */

                /* Get the next property node in the skip list */
                curr_node=H5SL_next(curr_node);
            } /* end while */
        } /* end if */

        /* Go up to parent class */
        tclass=tclass->parent;
    } /* end while */

    /* Decrement class's dependant property list value! */
    if(H5P_access_class(plist->pclass,H5P_MOD_DEC_LST)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL, "Can't decrement class ref count");

    /* Free the list of 'seen' properties */
    H5SL_close(seen);
    seen=NULL;

    /* Free the list of deleted property names */
    H5SL_destroy(plist->del,H5P_free_del_name_cb,NULL);

    /* Free the properties */
    H5SL_destroy(plist->props,H5P_free_prop_cb,&make_cb);

    /* Destroy property list object */
    H5FL_FREE(H5P_genplist_t,plist);

done:
    /* Release the skip list of 'seen' properties */
    if(seen!=NULL)
        H5SL_close(seen);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_close() */


/*--------------------------------------------------------------------------
 NAME
    H5Pclose
 PURPOSE
    Routine to close a property list.
 USAGE
    herr_t H5Pclose(plist_id)
        hid_t plist_id;       IN: Property list to close
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
        Closes a property list.  If a 'close' callback exists for the property
    list class, it is called before the property list is destroyed.  If 'close'
    callbacks exist for any individual properties in the property list, they are
    called after the class 'close' callback.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Pclose(hid_t plist_id)
{
    herr_t ret_value=SUCCEED;      /* return value */

    FUNC_ENTER_API(H5Pclose, FAIL);
    H5TRACE1("e","i",plist_id);

    if (plist_id==H5P_DEFAULT)
        HGOTO_DONE(SUCCEED);

    /* Check arguments. */
    if (H5I_GENPROP_LST != H5I_get_type(plist_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list");

    /* Close the property list */
    if (H5I_dec_ref(plist_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTFREE, FAIL, "can't close");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pclose() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_class_name
 PURPOSE
    Internal routine to query the name of a generic property list class
 USAGE
    char *H5P_get_class_name(pclass)
        H5P_genclass_t *pclass;    IN: Property list class to check
 RETURNS
    Success: Pointer to a malloc'ed string containing the class name
    Failure: NULL
 DESCRIPTION
        This routine retrieves the name of a generic property list class.
    The pointer to the name must be free'd by the user for successful calls.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
char *
H5P_get_class_name(H5P_genclass_t *pclass)
{
    char *ret_value;      /* return value */

    FUNC_ENTER_NOAPI(H5P_get_class_name, NULL);

    assert(pclass);

    /* Get class name */
    ret_value=H5MM_xstrdup(pclass->name);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_get_class_name() */


/*--------------------------------------------------------------------------
 NAME
    H5Pget_class_name
 PURPOSE
    Routine to query the name of a generic property list class
 USAGE
    char *H5Pget_class_name(pclass_id)
        hid_t pclass_id;         IN: Property class to query
 RETURNS
    Success: Pointer to a malloc'ed string containing the class name
    Failure: NULL
 DESCRIPTION
        This routine retrieves the name of a generic property list class.
    The pointer to the name must be free'd by the user for successful calls.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
char *
H5Pget_class_name(hid_t pclass_id)
{
    H5P_genclass_t  *pclass;    /* Property class to query */
    char *ret_value;       /* return value */

    FUNC_ENTER_API(H5Pget_class_name, NULL);

    /* Check arguments. */
    if (NULL == (pclass = H5I_object_verify(pclass_id, H5I_GENPROP_CLS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a property class");

    /* Get the property list class name */
    if ((ret_value=H5P_get_class_name(pclass))==NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, NULL, "unable to query name of class");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pget_class_name() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_class_path
 PURPOSE
    Internal routine to query the full path of a generic property list class
 USAGE
    char *H5P_get_class_name(pclass)
        H5P_genclass_t *pclass;    IN: Property list class to check
 RETURNS
    Success: Pointer to a malloc'ed string containing the full path of class
    Failure: NULL
 DESCRIPTION
        This routine retrieves the full path name of a generic property list
    class, starting with the root of the class hierarchy.
    The pointer to the name must be free'd by the user for successful calls.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
char *
H5P_get_class_path(H5P_genclass_t *pclass)
{
    char *par_path;     /* Parent class's full path */
    size_t par_path_len;/* Parent class's full path's length */
    size_t my_path_len; /* This class's name's length */
    char *ret_value;    /* return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_get_class_path);

    assert(pclass);

    /* Recursively build the full path */
    if(pclass->parent!=NULL) {
        /* Get the parent class's path */
        par_path=H5P_get_class_path(pclass->parent);
        if(par_path!=NULL) {
            /* Get the string lengths we need to allocate space */
            par_path_len=HDstrlen(par_path);
            my_path_len=HDstrlen(pclass->name);

            /* Allocate enough space for the parent class's path, plus the '/'
             * separator, this class's name and the string terminator
             */
            if(NULL==(ret_value=H5MM_malloc(par_path_len+1+my_path_len+1)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for class name");

            /* Build the full path for this class */
            HDstrcpy(ret_value,par_path);
            HDstrcat(ret_value,"/");
            HDstrcat(ret_value,pclass->name);

            /* Free the parent class's path */
            H5MM_xfree(par_path);
        } /* end if */
        else
            ret_value=H5MM_xstrdup(pclass->name);
    } /* end if */
    else
        ret_value=H5MM_xstrdup(pclass->name);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_get_class_path() */


/*--------------------------------------------------------------------------
 NAME
    H5P_open_class_path
 PURPOSE
    Internal routine to open [a copy of] a class with its full path name
 USAGE
    H5P_genclass_t *H5P_open_class_path(path)
        const char *path;       IN: Full path name of class to open [copy of]
 RETURNS
    Success: Pointer to a generic property class object
    Failure: NULL
 DESCRIPTION
    This routine opens [a copy] of the class indicated by the full path.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5P_genclass_t *
H5P_open_class_path(const char *path)
{
    char *tmp_path=NULL;        /* Temporary copy of the path */
    char *curr_name;            /* Pointer to current component of path name */
    char *delimit;              /* Pointer to path delimiter during traversal */
    H5P_genclass_t *curr_class; /* Pointer to class during path traversal */
    H5P_genclass_t *ret_value;  /* Return value */
    H5P_check_class_t check_info;   /* Structure to hold the information for checking duplicate names */

    FUNC_ENTER_NOAPI_NOINIT(H5P_open_class_path);

    assert(path);

    /* Duplicate the path to use */
    tmp_path=HDstrdup(path);
    assert(tmp_path);

    /* Find the generic property class with this full path */
    curr_name=tmp_path;
    curr_class=NULL;
    while((delimit=HDstrchr(curr_name,'/'))!=NULL) {
        /* Change the delimiter to terminate the string */
        *delimit='\0';

        /* Set up the search structure */
        check_info.parent=curr_class;
        check_info.name=curr_name;

        /* Find the class with this name & parent by iterating over the open classes */
        if((curr_class=H5I_search(H5I_GENPROP_CLS,H5P_check_class,&check_info))==NULL)
            HGOTO_ERROR (H5E_PLIST, H5E_NOTFOUND, NULL, "can't locate class");

        /* Advance the pointer in the path to the start of the next component */
        curr_name=delimit+1;
    } /* end while */

    /* Should be pointing to the last component in the path name now... */

    /* Set up the search structure */
    check_info.parent=curr_class;
    check_info.name=curr_name;

    /* Find the class with this name & parent by iterating over the open classes */
    if((curr_class=H5I_search(H5I_GENPROP_CLS,H5P_check_class,&check_info))==NULL)
        HGOTO_ERROR (H5E_PLIST, H5E_NOTFOUND, NULL, "can't locate class");

    /* Copy it */
    if((ret_value=H5P_copy_pclass(curr_class))==NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCOPY, NULL, "can't copy property class");

done:
    /* Free the duplicated path */
    H5MM_xfree(tmp_path);

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_open_class_path() */


/*--------------------------------------------------------------------------
 NAME
    H5P_get_class_parent
 PURPOSE
    Internal routine to query the parent class of a generic property class
 USAGE
    H5P_genclass_t *H5P_get_class_parent(pclass)
        H5P_genclass_t *pclass;    IN: Property class to check
 RETURNS
    Success: Pointer to the parent class of a property class
    Failure: NULL
 DESCRIPTION
    This routine retrieves a pointer to the parent class for a property class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static H5P_genclass_t *
H5P_get_class_parent(H5P_genclass_t *pclass)
{
    H5P_genclass_t *ret_value;      /* return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5P_get_class_parent);

    assert(pclass);

    /* Get property size */
    ret_value=pclass->parent;

    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_get_class_parent() */


/*--------------------------------------------------------------------------
 NAME
    H5Pget_class_parent
 PURPOSE
    routine to query the parent class of a generic property class
 USAGE
    hid_t H5Pget_class_parent(pclass_id)
        hid_t pclass_id;         IN: Property class to query
 RETURNS
    Success: ID of parent class object
    Failure: NULL
 DESCRIPTION
    This routine retrieves an ID for the parent class of a property class.

 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
hid_t
H5Pget_class_parent(hid_t pclass_id)
{
    H5P_genclass_t  *pclass;    /* Property class to query */
    H5P_genclass_t  *parent=NULL;   /* Parent's property class */
    hid_t ret_value;       /* return value */

    FUNC_ENTER_API(H5Pget_class_parent, FAIL);
    H5TRACE1("i","i",pclass_id);

    /* Check arguments. */
    if (NULL == (pclass = H5I_object_verify(pclass_id, H5I_GENPROP_CLS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property class");

    /* Retrieve the property class's parent */
    if ((parent=H5P_get_class_parent(pclass))==NULL)
        HGOTO_ERROR(H5E_PLIST, H5E_NOTFOUND, FAIL, "unable to query class of property list");

    /* Increment the outstanding references to the class object */
    if(H5P_access_class(parent,H5P_MOD_INC_REF)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_CANTINIT, FAIL,"Can't increment class ID ref count");

    /* Get an atom for the class */
    if ((ret_value = H5I_register(H5I_GENPROP_CLS, parent))<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTREGISTER, FAIL, "unable to atomize property list class");

done:
    if (ret_value<0 && parent)
        H5P_close_class(parent);

    FUNC_LEAVE_API(ret_value);
}   /* H5Pget_class_parent() */


/*--------------------------------------------------------------------------
 NAME
    H5P_close_class
 PURPOSE
    Internal routine to close a property list class.
 USAGE
    herr_t H5P_close_class(class)
        H5P_genclass_t *class;  IN: Property list class to close
 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Releases memory and de-attach a class from the property list class hierarchy.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5P_close_class(void *_pclass)
{
    H5P_genclass_t *pclass=(H5P_genclass_t *)_pclass;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_close_class);

    assert(pclass);

    /* Decrement the reference count & check if the object should go away */
    if(H5P_access_class(pclass,H5P_MOD_DEC_REF)<0)
        HGOTO_ERROR (H5E_PLIST, H5E_NOTFOUND, FAIL, "Can't decrement ID ref count");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5P_close_class() */


/*--------------------------------------------------------------------------
 NAME
    H5Pclose_class
 PURPOSE
    Close a property list class.
 USAGE
    herr_t H5Pclose_class(cls_id)
        hid_t cls_id;       IN: Property list class ID to class

 RETURNS
    Returns non-negative on success, negative on failure.
 DESCRIPTION
    Releases memory and de-attach a class from the property list class hierarchy.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5Pclose_class(hid_t cls_id)
{
    hid_t  ret_value = SUCCEED;    /* Return value      */

    FUNC_ENTER_API(H5Pclose_class, FAIL);
    H5TRACE1("e","i",cls_id);

    /* Check arguments */
    if (H5I_GENPROP_CLS != H5I_get_type(cls_id))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list class");

    /* Close the property list class */
    if (H5I_dec_ref(cls_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTFREE, FAIL, "can't close");

done:
    FUNC_LEAVE_API(ret_value);
}   /* H5Pclose_class() */
