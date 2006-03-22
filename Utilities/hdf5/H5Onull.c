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

/*-------------------------------------------------------------------------
 *
 * Created:             H5Onull.c
 *                      Aug  6 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             The null message.
 *
 * Modifications:       
 *
 *-------------------------------------------------------------------------
 */

#define H5O_PACKAGE	/*suppress error about including H5Opkg	  */

#include "H5private.h"
#include "H5Opkg.h"             /* Object header functions                  */

#define PABLO_MASK      H5O_null_mask

/* This message derives from H5O */
const H5O_class_t H5O_NULL[1] = {{
    H5O_NULL_ID,            /*message id number             */
    "null",                 /*message name for debugging    */
    0,                      /*native message size           */
    NULL,                   /*no decode method              */
    NULL,                   /*no encode method              */
    NULL,                   /*no copy method                */
    NULL,                   /*no size method                */
    NULL,                   /*no reset method               */
    NULL,                   /*no free method                */
    NULL,		    /*no file delete method         */
    NULL,		    /*no link method		    */
    NULL,		    /*no get share method	    */
    NULL,	            /*no set share method	    */
    NULL,                   /*no debug method               */
}};
