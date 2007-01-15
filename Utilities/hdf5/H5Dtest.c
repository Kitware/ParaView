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
 *              Thusdayr, May 27, 2004
 *
 * Purpose:  Dataset testing functions.
 */

#define H5D_PACKAGE    /*suppress error about including H5Dpkg    */
#define H5D_TESTING    /*suppress warning about H5D testing funcs*/


#include "H5private.h"    /* Generic Functions        */
#include "H5Dpkg.h"    /* Datasets         */
#include "H5Eprivate.h"    /* Error handling      */
#include "H5Iprivate.h"    /* ID Functions      */


/*--------------------------------------------------------------------------
 NAME
    H5D_layout_version_test
 PURPOSE
    Determine the storage layout version for a dataset's layout information
 USAGE
    herr_t H5D_layout_version_test(did, version)
        hid_t did;              IN: Dataset to query
        unsigned *version;      OUT: Pointer to location to place version info
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Checks the version of the storage layout information for a dataset.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    DO NOT USE THIS FUNCTION FOR ANYTHING EXCEPT TESTING
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5D_layout_version_test(hid_t did, unsigned *version)
{
    H5D_t  *dset;          /* Pointer to dataset to query */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_NOAPI(H5D_layout_version_test, FAIL);

    /* Check args */
    if (NULL==(dset=H5I_object_verify(did, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    if(version)
        *version=dset->shared->layout.version;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5D_layout_version_test() */


/*--------------------------------------------------------------------------
 NAME
    H5D_layout_contig_size_test
 PURPOSE
    Determine the size of a contiguous layout for a dataset's layout information
 USAGE
    herr_t H5D_layout_contig_size_test(did, size)
        hid_t did;              IN: Dataset to query
        hsize_t *size;          OUT: Pointer to location to place size info
 RETURNS
    Non-negative on success, negative on failure
 DESCRIPTION
    Checks the size of a contiguous dataset's storage.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
    DO NOT USE THIS FUNCTION FOR ANYTHING EXCEPT TESTING
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
herr_t
H5D_layout_contig_size_test(hid_t did, hsize_t *size)
{
    H5D_t  *dset;          /* Pointer to dataset to query */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_NOAPI(H5D_layout_contig_size_test, FAIL);

    /* Check args */
    if (NULL==(dset=H5I_object_verify(did, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset")

    if(size) {
        assert(dset->shared->layout.type==H5D_CONTIGUOUS);
        *size=dset->shared->layout.u.contig.size;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* H5D_layout_contig_size_test() */

