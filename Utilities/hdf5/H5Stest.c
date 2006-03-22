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
 *              Saturday, May 31, 2003
 *
 * Purpose:	Dataspace selection testing functions.
 */

#define H5S_PACKAGE		/*suppress error about including H5Spkg	  */
#define H5S_TESTING		/*suppress warning about H5S testing funcs*/

/* Pablo information */
/* (Put before include files to avoid problems with inline functions) */
#define PABLO_MASK      H5Stest_mask

#include "H5private.h"		/* Generic Functions			  */
#include "H5Eprivate.h"		/* Error handling		  */
#include "H5Iprivate.h"		/* ID Functions		  */
#include "H5Spkg.h"		/* Dataspace functions			  */

/* Interface initialization */
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;


/*--------------------------------------------------------------------------
 NAME
    H5S_select_shape_same_test
 PURPOSE
    Determine if two dataspace selections are the same shape
 USAGE
    htri_t H5S_select_shape_same_test(sid1, sid2)
        hid_t sid1;          IN: 1st dataspace to compare
        hid_t sid2;          IN: 2nd dataspace to compare
 RETURNS
    Non-negative TRUE/FALSE on success, negative on failure
 DESCRIPTION
    Checks to see if the current selection in the dataspaces are the same
    dimensionality and shape.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    DO NOT USE THIS FUNCTION FOR ANYTHING EXCEPT TESTING H5P_get_class_path()
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
htri_t
H5S_select_shape_same_test(hid_t sid1, hid_t sid2)
{
    H5S_t	*space1 = NULL;      /* Pointer to 1st dataspace */
    H5S_t	*space2 = NULL;      /* Pointer to 2nd dataspace */
    htri_t ret_value;        /* return value */

    FUNC_ENTER_NOAPI(H5S_select_shape_same_test, FAIL);

    /* Get dataspace structures */
    if (NULL == (space1=H5I_object_verify(sid1, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace");
    if (NULL == (space2=H5I_object_verify(sid2, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace");

    /* Check if the dataspace selections are the same shape */
    if ((ret_value=H5S_select_shape_same(space1,space2))<0)
        HGOTO_ERROR(H5E_DATASPACE, H5E_CANTCOMPARE, FAIL, "unable to compare dataspace selections");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5S_select_shape_same_test() */

