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
 * This file contains private information about the H5D module
 */
#ifndef _H5Aprivate_H
#define _H5Aprivate_H

#include "H5Apublic.h"
#include "H5Gprivate.h"

#define H5A_RESERVED_ATOMS  0
typedef struct H5A_t H5A_t;

/* Private headers needed by this file */

/* Functions defined in H5A.c */
__DLL__ H5A_t * H5A_copy(const H5A_t *old_attr);
__DLL__ herr_t H5A_close(H5A_t *attr);
__DLL__ H5G_entry_t *H5A_entof(H5A_t *attr);

#endif
