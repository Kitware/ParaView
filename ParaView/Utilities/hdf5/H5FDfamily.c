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
 * Programmer:	Robb Matzke <matzke@llnl.gov>
 *		Monday, November 10, 1997
 *
 * Purpose:	Implements a family of files that acts as a single hdf5
 *		file.  The purpose is to be able to split a huge file on a
 *		64-bit platform, transfer all the <2GB members to a 32-bit
 *		platform, and then access the entire huge file on the 32-bit
 *		platform.
 *
 *		All family members are logically the same size although their
 *		physical sizes may vary.  The logical member size is
 *		determined by looking at the physical size of the first member
 *		when the file is opened.  When creating a file family, the
 *		first member is created with a predefined physical size
 *		(actually, this happens when the file family is flushed, and
 *		can be quite time consuming on file systems that don't
 *		implement holes, like nfs).
 *		
 */
#include "H5private.h"		/*library functions			*/
#include "H5Eprivate.h"		/*error handling			*/
#include "H5Fprivate.h"		/*files					*/
#include "H5FDprivate.h"	/*file driver				  */
#include "H5FDfamily.h"         /* Family file driver */
#include "H5Iprivate.h"		/*object IDs				  */
#include "H5MMprivate.h"        /* Memory allocation */
#include "H5Pprivate.h"		/*property lists			*/


#undef MAX
#define MAX(X,Y)	((X)>(Y)?(X):(Y))
#undef MIN
#define MIN(X,Y)	((X)<(Y)?(X):(Y))

/* The driver identification number, initialized at runtime */
static hid_t H5FD_FAMILY_g = 0;

/* The description of a file belonging to this driver. */
typedef struct H5FD_family_t {
    H5FD_t	pub;		/*public stuff, must be first		*/
    hid_t	memb_fapl_id;	/*file access property list for members	*/
    hsize_t	memb_size;	/*maximum size of each member file	*/
    int		nmembs;		/*number of family members		*/
    int		amembs;		/*number of member slots allocated	*/
    H5FD_t	**memb;		/*dynamic array of member pointers	*/
    haddr_t	eoa;		/*end of allocated addresses		*/
    char	*name;		/*name generator printf format		*/
    unsigned	flags;		/*flags for opening additional members	*/
} H5FD_family_t;

/* Driver-specific file access properties */
typedef struct H5FD_family_fapl_t {
    hsize_t	memb_size;	/*size of each member			*/
    hid_t	memb_fapl_id;	/*file access property list of each memb*/
} H5FD_family_fapl_t;

/* Driver specific data transfer properties */
typedef struct H5FD_family_dxpl_t {
    hid_t	memb_dxpl_id;	/*data xfer property list of each memb	*/
} H5FD_family_dxpl_t;

/* Callback prototypes */
static void *H5FD_family_fapl_get(H5FD_t *_file);
static void *H5FD_family_fapl_copy(const void *_old_fa);
static herr_t H5FD_family_fapl_free(void *_fa);
static void *H5FD_family_dxpl_copy(const void *_old_dx);
static herr_t H5FD_family_dxpl_free(void *_dx);
static H5FD_t *H5FD_family_open(const char *name, unsigned flags,
				hid_t fapl_id, haddr_t maxaddr);
static herr_t H5FD_family_close(H5FD_t *_file);
static int H5FD_family_cmp(const H5FD_t *_f1, const H5FD_t *_f2);
static herr_t H5FD_family_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_family_get_eoa(H5FD_t *_file);
static herr_t H5FD_family_set_eoa(H5FD_t *_file, haddr_t eoa);
static haddr_t H5FD_family_get_eof(H5FD_t *_file);
static herr_t  H5FD_family_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle);
static herr_t H5FD_family_read(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr,
			       size_t size, void *_buf/*out*/);
static herr_t H5FD_family_write(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr,
				size_t size, const void *_buf);
static herr_t H5FD_family_flush(H5FD_t *_file, hid_t dxpl_id, unsigned closing);

/* The class struct */
static const H5FD_class_t H5FD_family_g = {
    "family",					/*name			*/
    HADDR_MAX,					/*maxaddr		*/
    H5F_CLOSE_WEAK,				/* fc_degree		*/
    NULL,					/*sb_size		*/
    NULL,					/*sb_encode		*/
    NULL,					/*sb_decode		*/
    sizeof(H5FD_family_fapl_t),			/*fapl_size		*/
    H5FD_family_fapl_get,			/*fapl_get		*/
    H5FD_family_fapl_copy,			/*fapl_copy		*/
    H5FD_family_fapl_free,			/*fapl_free		*/
    sizeof(H5FD_family_dxpl_t),			/*dxpl_size		*/
    H5FD_family_dxpl_copy,			/*dxpl_copy		*/
    H5FD_family_dxpl_free,			/*dxpl_free		*/
    H5FD_family_open,				/*open			*/
    H5FD_family_close,				/*close			*/
    H5FD_family_cmp,				/*cmp			*/
    H5FD_family_query,		                /*query			*/
    NULL,					/*alloc			*/
    NULL,					/*free			*/
    H5FD_family_get_eoa,			/*get_eoa		*/
    H5FD_family_set_eoa,			/*set_eoa		*/
    H5FD_family_get_eof,			/*get_eof		*/
    H5FD_family_get_handle,                     /*get_handle            */
    H5FD_family_read,				/*read			*/
    H5FD_family_write,				/*write			*/
    H5FD_family_flush,				/*flush			*/
    NULL,                                       /*lock                  */
    NULL,                                       /*unlock                */
    H5FD_FLMAP_SINGLE 				/*fl_map		*/
};

/* Interface initialization */
#define PABLO_MASK	H5FD_family_mask
#define INTERFACE_INIT	H5FD_family_init
static int interface_initialize_g = 0;


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_init
 *
 * Purpose:	Initialize this driver by registering the driver with the
 *		library.
 *
 * Return:	Success:	The driver ID for the family driver.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_family_init(void)
{
    hid_t ret_value=H5FD_FAMILY_g;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_init, FAIL);

    if (H5I_VFL!=H5Iget_type(H5FD_FAMILY_g))
        H5FD_FAMILY_g = H5FDregister(&H5FD_family_g);

    /* Set return value */
    ret_value=H5FD_FAMILY_g;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_fapl_family
 *
 * Purpose:	Sets the file access property list FAPL_ID to use the family
 *		driver. The MEMB_SIZE is the size in bytes of each file
 *		member (used only when creating a new file) and the
 *		MEMB_FAPL_ID is a file access property list to be used for
 *		each family member.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *		Raymond Lu 
 * 		Tuesday, Oct 23, 2001
 *		Changed the file access list to the new generic property 
 *		list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_family(hid_t fapl_id, hsize_t memb_size, hid_t memb_fapl_id)
{
    herr_t ret_value;
    H5FD_family_fapl_t	fa;
    H5P_genplist_t *plist;      /* Property list pointer */
    
    FUNC_ENTER_API(H5Pset_fapl_family, FAIL);
    H5TRACE3("e","ihi",fapl_id,memb_size,memb_fapl_id);
    
    /* Check arguments */
    if(TRUE != H5P_isa_class(fapl_id, H5P_FILE_ACCESS))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");
    if(H5P_DEFAULT == memb_fapl_id)
        memb_fapl_id = H5P_FILE_ACCESS_DEFAULT;
    else
        if(TRUE != H5P_isa_class(memb_fapl_id, H5P_FILE_ACCESS))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access list");

    /*
     * Initialize driver specific information. No need to copy it into the FA
     * struct since all members will be copied by H5P_set_driver().
     */
    fa.memb_size = memb_size;
    fa.memb_fapl_id = memb_fapl_id;

    if(NULL == (plist = H5I_object(fapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");
    ret_value= H5P_set_driver(plist, H5FD_FAMILY, &fa);

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_fapl_family
 *
 * Purpose:	Returns information about the family file access property
 *		list though the function arguments.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *		Raymond Lu 
 * 		Tuesday, Oct 23, 2001
 *		Changed the file access list to the new generic property 
 *		list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fapl_family(hid_t fapl_id, hsize_t *memb_size/*out*/,
		   hid_t *memb_fapl_id/*out*/)
{
    H5FD_family_fapl_t	*fa;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */
    
    FUNC_ENTER_API(H5Pget_fapl_family, FAIL);
    H5TRACE3("e","ixx",fapl_id,memb_size,memb_fapl_id);

    if(NULL == (plist = H5P_object_verify(fapl_id,H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access list"); 
    if (H5FD_FAMILY!=H5P_get_driver(plist))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver");
    if (NULL==(fa=H5P_get_driver_info(plist)))
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "bad VFL driver info");
    if (memb_size)
        *memb_size = fa->memb_size;
    if (memb_fapl_id) {
        if(NULL == (plist = H5I_object(fa->memb_fapl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access list"); 
        *memb_fapl_id = H5P_copy_plist(plist);
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_fapl_get
 *
 * Purpose:	Gets a file access property list which could be used to
 *		create an identical file.
 *
 * Return:	Success:	Ptr to new file access property list.
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Friday, August 13, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_family_fapl_get(H5FD_t *_file)
{
    H5FD_family_t	*file = (H5FD_family_t*)_file;
    H5FD_family_fapl_t	*fa = NULL;
    H5P_genplist_t *plist;      /* Property list pointer */
    void *ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_fapl_get, NULL);

    if (NULL==(fa=H5MM_calloc(sizeof(H5FD_family_fapl_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    fa->memb_size = file->memb_size;
    if(NULL == (plist = H5I_object(file->memb_fapl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list");
    fa->memb_fapl_id = H5P_copy_plist(plist);

    /* Set return value */
    ret_value=fa;

done:
    if(ret_value==NULL) {
        if(fa!=NULL)
            H5MM_xfree(fa);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_fapl_copy
 *
 * Purpose:	Copies the family-specific file access properties.
 *
 * Return:	Success:	Ptr to a new property list
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_family_fapl_copy(const void *_old_fa)
{
    const H5FD_family_fapl_t *old_fa = (const H5FD_family_fapl_t*)_old_fa;
    H5FD_family_fapl_t *new_fa = NULL;
    H5P_genplist_t *plist;      /* Property list pointer */
    void *ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_fapl_copy, NULL);

    if (NULL==(new_fa=H5MM_malloc(sizeof(H5FD_family_fapl_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    /* Copy the fields of the structure */
    memcpy(new_fa, old_fa, sizeof(H5FD_family_fapl_t));

    /* Deep copy the property list objects in the structure */
    if(old_fa->memb_fapl_id==H5P_FILE_ACCESS_DEFAULT)
        H5I_inc_ref(new_fa->memb_fapl_id);
    else {
        if(NULL == (plist = H5I_object(old_fa->memb_fapl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list");
        new_fa->memb_fapl_id = H5P_copy_plist(plist);
    } /* end else */

    /* Set return value */
    ret_value=new_fa;

done:
    if(ret_value==NULL) {
        if(new_fa!=NULL)
            H5MM_xfree(new_fa);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_fapl_free
 *
 * Purpose:	Frees the family-specific file access properties.
 *
 * Return:	Success:	0
 *
 *		Failure:	-1
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_family_fapl_free(void *_fa)
{
    H5FD_family_fapl_t	*fa = (H5FD_family_fapl_t*)_fa;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_fapl_free, FAIL);

    H5I_dec_ref(fa->memb_fapl_id);
    H5MM_xfree(fa);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_dxpl_copy
 *
 * Purpose:	Copes the family-specific data transfer properties.
 *
 * Return:	Success:	Ptr to new property list
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_family_dxpl_copy(const void *_old_dx)
{
    const H5FD_family_dxpl_t *old_dx = (const H5FD_family_dxpl_t*)_old_dx;
    H5FD_family_dxpl_t *new_dx = NULL;
    H5P_genplist_t *plist;      /* Property list pointer */
    void *ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_dxpl_copy, NULL);

    if (NULL==(new_dx=H5MM_malloc(sizeof(H5FD_family_dxpl_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    memcpy(new_dx, old_dx, sizeof(H5FD_family_dxpl_t));

    if(old_dx->memb_dxpl_id==H5P_DATASET_XFER_DEFAULT)
        H5I_inc_ref(new_dx->memb_dxpl_id);
    else {
        if(NULL == (plist = H5I_object(old_dx->memb_dxpl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list");
        new_dx->memb_dxpl_id = H5P_copy_plist(plist);
    } /* end else */

    /* Set return value */
    ret_value=new_dx;

done:
    if(ret_value==NULL) {
        if(new_dx!=NULL)
            H5MM_xfree(new_dx);
    } /* end if */
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_dxpl_free
 *
 * Purpose:	Frees the family-specific data transfer properties.
 *
 * Return:	Success:	0
 *
 *		Failure:	-1
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_family_dxpl_free(void *_dx)
{
    H5FD_family_dxpl_t	*dx = (H5FD_family_dxpl_t*)_dx;
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_dxpl_free, FAIL);

    H5I_dec_ref(dx->memb_dxpl_id);
    H5MM_xfree(dx);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_open
 *
 * Purpose:	Creates and/or opens a family of files as an HDF5 file.
 *
 * Return:	Success:	A pointer to a new file dat structure. The
 *				public fields will be initialized by the
 *				caller, which is always H5FD_open().
 *
 *		Failure:	NULL
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_family_open(const char *name, unsigned flags, hid_t fapl_id,
		 haddr_t maxaddr)
{
    H5FD_family_t	*file=NULL;
    H5FD_t     		*ret_value=NULL;
    char		memb_name[4096], temp[4096];
    hsize_t		eof;
    unsigned		t_flags = flags & ~H5F_ACC_CREAT;
    H5P_genplist_t      *plist;      /* Property list pointer */
    
    FUNC_ENTER_NOAPI(H5FD_family_open, NULL);

    /* Check arguments */
    if (!name || !*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid file name");
    if (0==maxaddr || HADDR_UNDEF==maxaddr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr");

    /* Initialize file from file access properties */
    if (NULL==(file=H5MM_calloc(sizeof(H5FD_family_t))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to allocate file struct");
    if (H5P_FILE_ACCESS_DEFAULT==fapl_id) {
        file->memb_fapl_id = H5P_FILE_ACCESS_DEFAULT;
        H5I_inc_ref(file->memb_fapl_id);
        file->memb_size = 1024*1024*1024; /*1GB*/
    } else {
        H5FD_family_fapl_t *fa;

        if(NULL == (plist = H5I_object(fapl_id)))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list");
        fa = H5P_get_driver_info(plist);
        if(fa->memb_fapl_id==H5P_FILE_ACCESS_DEFAULT) {
            H5I_inc_ref(fa->memb_fapl_id);
            file->memb_fapl_id = fa->memb_fapl_id;
        } /* end if */
        else {
            if(NULL == (plist = H5I_object(fa->memb_fapl_id)))
                HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file access property list");
            file->memb_fapl_id = H5P_copy_plist(plist);
        } /* end else */
        file->memb_size = fa->memb_size;
    }
    file->name = H5MM_strdup(name);
    file->flags = flags;
    
    /* Check that names are unique */
    sprintf(memb_name, name, 0);
    sprintf(temp, name, 1);
    if (!strcmp(memb_name, temp))
        HGOTO_ERROR(H5E_FILE, H5E_FILEEXISTS, NULL, "file names not unique");

    /* Open all the family members */
    while (1) {
        sprintf(memb_name, name, file->nmembs);

        /* Enlarge member array */
        if (file->nmembs>=file->amembs) {
            int n = MAX(64, 2*file->amembs);
            H5FD_t **x = H5MM_realloc(file->memb, n*sizeof(H5FD_t*));

            if (!x)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "unable to reallocate members");
            file->amembs = n;
            file->memb = x;
        }
        
        /*
         * Attempt to open file. If the first file cannot be opened then fail;
         * otherwise an open failure means that we've reached the last member.
         * Allow H5F_ACC_CREAT only on the first family member.
         */
        H5E_BEGIN_TRY {
            file->memb[file->nmembs] = H5FDopen(memb_name,
                            0==file->nmembs?flags:t_flags,
                            file->memb_fapl_id,
                            HADDR_UNDEF);
        } H5E_END_TRY;
        if (!file->memb[file->nmembs]) {
            if (0==file->nmembs)
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to open member file");
            H5Eclear();
            break;
        }
        file->nmembs++;
    }
    
    /*
     * The size of the first member determines the size of all the members,
     * but if the size of the first member is zero then use the member size
     * from the file access property list.
     */
    if ((eof=H5FDget_eof(file->memb[0])))
        file->memb_size = eof;

    ret_value=(H5FD_t *)file;

done:
    /* Cleanup and fail */
    if (ret_value==NULL && file!=NULL) {
        int i;

        for (i=0; i<file->nmembs; i++)
            if (file->memb[i])
                H5FDclose(file->memb[i]);
        if (file->memb)
            H5MM_xfree(file->memb);
        H5I_dec_ref(file->memb_fapl_id);
        if (file->name)
            H5MM_xfree(file->name);
        H5MM_xfree(file);
    }
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_close
 *
 * Purpose:	Closes a family of files.
 *
 * Return:	Success:	Non-negative
 *
 *		Failure:	Negative with as many members closed as
 *				possible. The only subsequent operation
 *				permitted on the file is a close operation.
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_family_close(H5FD_t *_file)
{
    H5FD_family_t	*file = (H5FD_family_t*)_file;
    int			i, nerrors=0;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_close, FAIL);

    /* Close as many members as possible */
    for (i=0; i<file->nmembs; i++) {
        if (file->memb[i]) {
                if (H5FDclose(file->memb[i])<0) {
                nerrors++;
            } else {
                file->memb[i] = NULL;
            }
        }
    }
    if (nerrors)
        HGOTO_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, "unable to close member files");

    /* Clean up other stuff */
    H5I_dec_ref(file->memb_fapl_id);
    if (file->memb)
        H5MM_xfree(file->memb);
    if (file->name)
        H5MM_xfree(file->name);
    H5MM_xfree(file);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_cmp
 *
 * Purpose:	Compares two file families to see if they are the same. It
 *		does this by comparing the first member of the two families.
 *
 * Return:	Success:	like strcmp()
 *
 *		Failure:	never fails (arguments were checked by the
 *				caller).
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD_family_cmp(const H5FD_t *_f1, const H5FD_t *_f2)
{
    const H5FD_family_t	*f1 = (const H5FD_family_t*)_f1;
    const H5FD_family_t	*f2 = (const H5FD_family_t*)_f2;
    int ret_value=(H5FD_VFD_DEFAULT);

    FUNC_ENTER_NOAPI(H5FD_family_cmp, H5FD_VFD_DEFAULT);

    assert(f1->nmembs>=1 && f1->memb[0]);
    assert(f2->nmembs>=1 && f2->memb[0]);
    
    ret_value= H5FDcmp(f1->memb[0], f2->memb[0]);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_query
 *
 * Purpose:	Set the flags that this VFL driver is capable of supporting.
 *              (listed in H5FDpublic.h)
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Friday, August 25, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_family_query(const H5FD_t UNUSED * _f, unsigned long *flags /* out */)
{
    herr_t ret_value=SUCCEED;

    FUNC_ENTER_NOAPI(H5FD_family_query, FAIL);

    /* Set the VFL feature flags that this driver supports */
    if(flags) {
        *flags=0;
        *flags|=H5FD_FEAT_AGGREGATE_METADATA; /* OK to aggregate metadata allocations */
        *flags|=H5FD_FEAT_ACCUMULATE_METADATA; /* OK to accumulate metadata for faster writes */
        *flags|=H5FD_FEAT_DATA_SIEVE;       /* OK to perform data sieving for faster raw data reads & writes */
        *flags|=H5FD_FEAT_AGGREGATE_SMALLDATA; /* OK to aggregate "small" raw data allocations */
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_get_eoa
 *
 * Purpose:	Returns the end-of-address marker for the file. The EOA
 *		marker is the first address past the last byte allocated in
 *		the format address space.
 *
 * Return:	Success:	The end-of-address-marker
 *
 *		Failure:	HADDR_UNDEF
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_family_get_eoa(H5FD_t *_file)
{
    H5FD_family_t	*file = (H5FD_family_t*)_file;
    haddr_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_get_eoa, HADDR_UNDEF);

    /* Set return value */
    ret_value=file->eoa;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_set_eoa
 *
 * Purpose:	Set the end-of-address marker for the file.
 *
 * Return:	Success:	0
 *
 *		Failure:	-1
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_family_set_eoa(H5FD_t *_file, haddr_t eoa)
{
    H5FD_family_t	*file = (H5FD_family_t*)_file;
    haddr_t		addr=eoa;
    int			i;
    char		memb_name[4096];
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_set_eoa, FAIL);

    for (i=0; addr || i<file->nmembs; i++) {

        /* Enlarge member array */
        if (i>=file->amembs) {
            int n = MAX(64, 2*file->amembs);
            H5FD_t **x = H5MM_realloc(file->memb, n*sizeof(H5FD_t*));
            if (!x)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "unable to allocate memory block");
            file->amembs = n;
            file->memb = x;
            file->nmembs = i;
        }

        /* Create another file if necessary */
        if (i>=file->nmembs || !file->memb[i]) {
            file->nmembs = MAX(file->nmembs, i+1);
            sprintf(memb_name, file->name, i);
            H5E_BEGIN_TRY {
            H5_CHECK_OVERFLOW(file->memb_size,hsize_t,haddr_t);
            file->memb[i] = H5FDopen(memb_name, file->flags|H5F_ACC_CREAT,
                         file->memb_fapl_id, (haddr_t)file->memb_size);
            } H5E_END_TRY;
            if (NULL==file->memb[i])
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to open member file");
        }
        
        /* Set the EOA marker for the member */
        H5_CHECK_OVERFLOW(file->memb_size,hsize_t,haddr_t);
        if (addr>(haddr_t)file->memb_size) {
            H5FDset_eoa(file->memb[i], (haddr_t)file->memb_size);
            addr -= file->memb_size;
        } else {
            H5FDset_eoa(file->memb[i], addr);
            addr = 0;
        }
    }

    file->eoa = eoa;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_get_eof
 *
 * Purpose:	Returns the end-of-file marker, which is the greater of
 *		either the total family size or the current EOA marker.
 *
 * Return:	Success:	End of file address, the first address past
 *				the end of the family of files or the current
 *				EOA, whichever is larger.
 *
 *		Failure:      	HADDR_UNDEF
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_family_get_eof(H5FD_t *_file)
{
    H5FD_family_t	*file = (H5FD_family_t*)_file;
    haddr_t		eof=0;
    int			i;
    haddr_t ret_value;   /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_get_eof, HADDR_UNDEF);

    /*
     * Find the last member that has a non-zero EOF and break out of the loop
     * with `i' equal to that member. If all members have zero EOF then exit
     * loop with i==0.
     */
    for (i=file->nmembs-1; i>=0; --i) {
        if ((eof=H5FDget_eof(file->memb[i])))
            break;
        if (0==i)
            break;
    }

    /*
     * The file size is the number of members before the i'th member plus the
     * size of the i'th member.
     */
    eof += i*file->memb_size;

    /* Set return value */
    ret_value=MAX(eof, file->eoa);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:       H5FD_family_get_handle
 * 
 * Purpose:        Returns the file handle of FAMILY file driver.
 * 
 * Returns:        Non-negative if succeed or negative if fails.
 * 
 * Programmer:     Raymond Lu
 *                 Sept. 16, 2002
 *  
 * Modifications:
 *  
 *-------------------------------------------------------------------------
 */ 
static herr_t  
H5FD_family_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle)
{   
    H5FD_family_t       *file = (H5FD_family_t *)_file;
    H5P_genplist_t      *plist;
    hsize_t             offset;
    int                 memb;
    herr_t              ret_value;
                              
    FUNC_ENTER_NOAPI(H5FD_family_get_handle, FAIL);

    /* Get the plist structure and family offset */
    if(NULL == (plist = H5P_object_verify(fapl, H5P_FILE_ACCESS)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");
    if(H5P_get(plist, H5F_ACS_FAMILY_OFFSET_NAME, &offset) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get offset for family driver");

    if(offset>(file->memb_size*file->nmembs))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "offset is bigger than file size");
    memb = (int)(offset/file->memb_size);

    ret_value = H5FD_get_vfd_handle(file->memb[memb], fapl, file_handle);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_read
 *
 * Purpose:	Reads SIZE bytes of data from FILE beginning at address ADDR
 *		into buffer BUF according to data transfer properties in
 *		DXPL_ID.
 *
 * Return:	Success:	Zero. Result is stored in caller-supplied
 *				buffer BUF.
 *
 *		Failure:	-1, contents of buffer BUF are undefined.
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_family_read(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
		 void *_buf/*out*/)
{
    H5FD_family_t	*file = (H5FD_family_t*)_file;
    unsigned char	*buf = (unsigned char*)_buf;
    hid_t		memb_dxpl_id = H5P_DATASET_XFER_DEFAULT;
    int			i;
    haddr_t		sub;
    size_t		req;
    hsize_t             tempreq;
    H5P_genplist_t      *plist;      /* Property list pointer */
    herr_t              ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_read, FAIL);

    /*
     * Get the member data transfer property list. If the transfer property
     * list does not belong to this driver then assume defaults
     */
    if(NULL == (plist = H5I_object(dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");
    if (H5P_DATASET_XFER_DEFAULT!=dxpl_id && H5FD_FAMILY==H5P_get_driver(plist)) {
        H5FD_family_dxpl_t *dx = H5P_get_driver_info(plist);

        assert(TRUE==H5P_isa_class(dxpl_id, H5P_DATASET_XFER));
        assert(dx);
        memb_dxpl_id = dx->memb_dxpl_id;
    }

    /* Read from each member */
    while (size>0) {
        H5_ASSIGN_OVERFLOW(i,addr /file->memb_size,hsize_t,int);

        sub = addr % file->memb_size;

	/* This check is for mainly for IA32 architecture whose size_t's size
	 * is 4 bytes, to prevent overflow when user application is trying to 
	 * write files bigger than 4GB. */
        tempreq = file->memb_size-sub;
  	if(tempreq > SIZET_MAX)
	    tempreq = SIZET_MAX;
        req = MIN(size, (size_t)tempreq);

        assert(i<file->nmembs);

        if (H5FDread(file->memb[i], type, memb_dxpl_id, sub, req, buf)<0)
            HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL, "member file read failed");

        addr += req;
        buf += req;
        size -= req;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_write
 *
 * Purpose:	Writes SIZE bytes of data to FILE beginning at address ADDR
 *		from buffer BUF according to data transfer properties in
 *		DXPL_ID.
 *
 * Return:	Success:	Zero
 *
 *		Failure:	-1
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_family_write(H5FD_t *_file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
		  const void *_buf)
{
    H5FD_family_t	*file = (H5FD_family_t*)_file;
    const unsigned char	*buf = (const unsigned char*)_buf;
    hid_t		memb_dxpl_id = H5P_DATASET_XFER_DEFAULT;
    int			i;
    haddr_t		sub;
    size_t		req;
    hsize_t             tempreq;
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_write, FAIL);

    /*
     * Get the member data transfer property list. If the transfer property
     * list does not belong to this driver then assume defaults.
     */
    if(NULL == (plist = H5I_object(dxpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access property list");
    if (H5P_DATASET_XFER_DEFAULT!=dxpl_id && H5FD_FAMILY==H5P_get_driver(plist)) {
        H5FD_family_dxpl_t *dx = H5P_get_driver_info(plist);

        assert(TRUE==H5P_isa_class(dxpl_id, H5P_DATASET_XFER));
        assert(dx);
        memb_dxpl_id = dx->memb_dxpl_id;
    }

    /* Write to each member */
    while (size>0) {
        H5_ASSIGN_OVERFLOW(i,addr /file->memb_size,hsize_t,int);

        sub = addr % file->memb_size;

        /* This check is for mainly for IA32 architecture whose size_t's size
         * is 4 bytes, to prevent overflow when user application is trying to 
         * write files bigger than 4GB. */
        tempreq = file->memb_size-sub;
	if(tempreq > SIZET_MAX)
	    tempreq = SIZET_MAX;
        req = MIN(size, (size_t)tempreq);

        assert(i<file->nmembs);

        if (H5FDwrite(file->memb[i], type, memb_dxpl_id, sub, req, buf)<0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "member file write failed");

        addr += req;
        buf += req;
        size -= req;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5FD_family_flush
 *
 * Purpose:	Flushes all family members.
 *
 * Return:	Success:	0
 *
 *		Failure:	-1, as many files flushed as possible.
 *
 * Programmer:	Robb Matzke
 *              Wednesday, August  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_family_flush(H5FD_t *_file, hid_t dxpl_id, unsigned closing)
{
    H5FD_family_t	*file = (H5FD_family_t*)_file;
    int			i, nerrors=0;
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(H5FD_family_flush, FAIL);

    for (i=0; i<file->nmembs; i++)
        if (file->memb[i] && H5FDflush(file->memb[i], dxpl_id, closing)<0)
            nerrors++;

    if (nerrors)
        HGOTO_ERROR(H5E_IO, H5E_BADVALUE, FAIL, "unable to flush member files");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}
