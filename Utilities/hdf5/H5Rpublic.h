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
 * This file contains public declarations for the H5S module.
 */
#ifndef _H5Rpublic_H
#define _H5Rpublic_H

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Gpublic.h"
#include "H5Ipublic.h"

/*
 * Reference types allowed.
 */
typedef enum {
    H5R_BADTYPE     =   (-1),   /*invalid Reference Type                     */
    H5R_OBJECT,                 /*Object reference                           */
    H5R_DATASET_REGION,         /*Dataset Region Reference                   */
    H5R_INTERNAL,               /*Internal Reference                         */
    H5R_MAXTYPE                 /*highest type (Invalid as true type)	     */
} H5R_type_t;

#ifdef LATER
/* Generic reference structure for user's code */
typedef struct {
    unsigned long oid[2];       /* OID of object referenced */
    unsigned long region[2];    /* heap ID of region in object */
    unsigned long file[2];      /* heap ID of external filename */
} href_t;
#endif /* LATER */

/* Note! Be careful with the sizes of the references because they should really
 * depend on the run-time values in the file.  Unfortunately, the arrays need
 * to be defined at compile-time, so we have to go with the worst case sizes for
 * them.  -QAK
 */
#define H5R_OBJ_REF_BUF_SIZE    sizeof(haddr_t)
/* Object reference structure for user's code */
typedef struct {
    unsigned char oid[H5R_OBJ_REF_BUF_SIZE];    /* Buffer to store OID of object referenced */
                                /* Needs to be large enough to store largest haddr_t in a worst case machine (ie. 8 bytes currently) */
} hobj_ref_t;

#define H5R_DSET_REG_REF_BUF_SIZE    (sizeof(haddr_t)+sizeof(int))
/* Dataset Region reference structure for user's code */
typedef struct {
    unsigned char heapid[H5R_DSET_REG_REF_BUF_SIZE];    /* Buffer to store heap ID and index */
                                /* Needs to be large enough to store largest haddr_t in a worst case machine (ie. 8 bytes currently) plus an int (4 bytes typically, but could be 8 bytes) */
} hdset_reg_ref_t;

/* Publicly visible datastructures */

#ifdef __cplusplus
extern "C" {
#endif

/* Functions in H5R.c */
H5_DLL herr_t H5Rcreate(void *ref, hid_t loc_id, const char *name,
			 H5R_type_t ref_type, hid_t space_id);
H5_DLL hid_t H5Rdereference(hid_t dataset, H5R_type_t ref_type, void *ref);
H5_DLL hid_t H5Rget_region(hid_t dataset, H5R_type_t ref_type, void *ref);
#ifdef H5_WANT_H5_V1_4_COMPAT
H5_DLL int H5Rget_object_type(hid_t dataset, void *_ref);
H5_DLL int H5Rget_obj_type(hid_t id, H5R_type_t ref_type, void *_ref);
#else /* H5_WANT_H5_V1_4_COMPAT */
H5_DLL H5G_obj_t H5Rget_obj_type(hid_t id, H5R_type_t ref_type, void *_ref);
#endif /* H5_WANT_H5_V1_4_COMPAT */

#ifdef __cplusplus
}
#endif

#endif  /* _H5Rpublic_H */
