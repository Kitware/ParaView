/****************************************************************************
 * NCSA HDF                                                                 *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 *                                                                          *
 * For conditions of distribution and use, see the accompanying             *
 * hdf/COPYING file.                                                        *
 *                                                                          *
 ****************************************************************************/

/*
 * This file contains public declarations for the H5S module.
 */
#ifndef _H5Rpublic_H
#define _H5Rpublic_H

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Ipublic.h"

/*
 * Reference types allowed.
 */
typedef enum {
    H5R_BADTYPE     =   (-1),   /*invalid Reference Type                     */
    H5R_OBJECT,                 /*Object reference                           */
    H5R_DATASET_REGION,         /*Dataset Region Reference                   */
    H5R_INTERNAL,               /*Internal Reference                         */
    H5R_MAXTYPE                 /*highest type (Invalid as true type)        */
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
 * to be defined at run-time, so we have to go with the worst case sizes for
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
__DLL__ herr_t H5Rcreate(void *ref, hid_t loc_id, const char *name,
                         H5R_type_t ref_type, hid_t space_id);
__DLL__ hid_t H5Rdereference(hid_t dataset, H5R_type_t ref_type, void *ref);
__DLL__ hid_t H5Rget_region(hid_t dataset, H5R_type_t ref_type, void *ref);
__DLL__ int H5Rget_object_type(hid_t dataset, void *_ref);

#ifdef __cplusplus
}
#endif

#endif  /* _H5Rpublic_H */
