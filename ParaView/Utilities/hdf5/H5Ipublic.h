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
 * This file contains function prototypes for each exported function in
 * the H5I module.
 */
#ifndef _H5Ipublic_H
#define _H5Ipublic_H

/* Public headers needed by this file */
#include "H5public.h"

/*
 * Group values allowed.  Start with `1' instead of `0' because it makes the
 * tracing output look better when hid_t values are large numbers.  Change the
 * GROUP_BITS in H5I.c if the MAXID gets larger than 32 (an assertion will
 * fail otherwise).
 */
typedef enum {
    H5I_BADID           = (-1), /*invalid Group                             */
    H5I_FILE            = 1,    /*group ID for File objects                 */
    H5I_FILE_CLOSING,           /*files pending close due to open objhdrs   */
    H5I_TEMPLATE_0,                 /*group ID for Template objects                 */
    H5I_TEMPLATE_1,             /*group ID for Template objects             */
    H5I_TEMPLATE_2,             /*group ID for Template objects             */
    H5I_TEMPLATE_3,             /*group ID for Template objects             */
    H5I_TEMPLATE_4,             /*group ID for Template objects             */
    H5I_TEMPLATE_5,             /*group ID for Template objects             */
    H5I_TEMPLATE_6,             /*group ID for Template objects             */
    H5I_TEMPLATE_7,             /*group ID for Template objects             */
    H5I_TEMPLATE_MAX,       /*not really a group ID                             */
    H5I_GROUP,                  /*group ID for Group objects                */
    H5I_DATATYPE,               /*group ID for Datatype objects             */
    H5I_DATASPACE,              /*group ID for Dataspace objects            */
    H5I_DATASET,                /*group ID for Dataset objects              */
    H5I_ATTR,                   /*group ID for Attribute objects            */
    H5I_TEMPBUF,                /*group ID for Temporary buffer objects */
    H5I_REFERENCE,              /*group ID for Reference objects            */
    H5I_VFL,                        /*group ID for virtual file layer       */
    H5I_GENPROP_CLS,        /*group ID for generic property list classes */
    H5I_GENPROP_LST,        /*group ID for generic property lists   */
    
    H5I_NGROUPS                 /*number of valid groups, MUST BE LAST!     */
} H5I_type_t;

/* Type of atoms to return to users */
typedef int hid_t;

#ifdef __cplusplus
extern "C" {
#endif

__DLL__ H5I_type_t H5Iget_type(hid_t id);


#ifdef __cplusplus
}
#endif
#endif
