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

/*-----------------------------------------------------------------------------
 * File:    H5Iprivate.h
 * Purpose: header file for ID API
 *---------------------------------------------------------------------------*/

/* avoid re-inclusion */
#ifndef _H5Iprivate_H
#define _H5Iprivate_H

/* Include package's public header */
#include "H5Ipublic.h"

/* Private headers needed by this file */
#include "H5private.h"

/* Default sizes of the hash-tables for various atom groups */
#define H5I_ERRSTACK_HASHSIZE		64
#define H5I_FILEID_HASHSIZE		64
#define H5I_TEMPID_HASHSIZE		64
#define H5I_DATATYPEID_HASHSIZE		64
#define H5I_DATASPACEID_HASHSIZE	64
#define H5I_DATASETID_HASHSIZE		64
#define H5I_OID_HASHSIZE		64
#define H5I_GROUPID_HASHSIZE		64
#define H5I_ATTRID_HASHSIZE		64
#define H5I_TEMPBUFID_HASHSIZE		64
#define H5I_REFID_HASHSIZE		64
#define H5I_VFL_HASHSIZE		64
#define H5I_GENPROPCLS_HASHSIZE		64
#define H5I_GENPROPOBJ_HASHSIZE		128

/*
 * Function for freeing objects. This function will be called with an object
 * ID group number (object type) and a pointer to the object. The function
 * should free the object and return non-negative to indicate that the object
 * can be removed from the ID group. If the function returns negative
 * (failure) then the object will remain in the ID group.
 */
typedef herr_t (*H5I_free_t)(void*);

/* Type of the function to compare objects & keys */
typedef int (*H5I_search_func_t)(void *obj, hid_t id, void *key);

/* Private Functions in H5I.c */
H5_DLL int H5I_init_group(H5I_type_t grp, size_t hash_size, unsigned reserved,
			    H5I_free_t func);
H5_DLL int H5I_nmembers(H5I_type_t grp);
H5_DLL herr_t H5I_clear_group(H5I_type_t grp, hbool_t force);
H5_DLL herr_t H5I_destroy_group(H5I_type_t grp);
H5_DLL hid_t H5I_register(H5I_type_t grp, void *object);
H5_DLL void *H5I_object(hid_t id);
H5_DLL void *H5I_object_verify(hid_t id, H5I_type_t id_type);
H5_DLL H5I_type_t H5I_get_type(hid_t id);
H5_DLL void *H5I_remove(hid_t id);
H5_DLL void *H5I_search(H5I_type_t grp, H5I_search_func_t func, void *key);
H5_DLL int H5I_inc_ref(hid_t id);
H5_DLL int H5I_dec_ref(hid_t id);
#endif
