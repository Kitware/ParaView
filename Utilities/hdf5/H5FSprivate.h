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
 *  Header file for function stacks, etc.
 */
#ifndef _H5FSprivate_H
#define _H5FSprivate_H

#ifdef NOT_YET
#include "H5FSpublic.h"
#endif /* NOT_YET */

/* Private headers needed by this file */
#include "H5private.h"

#define H5FS_NSLOTS	32	/*number of slots in an function stack	     */

/* A function stack */
typedef struct H5FS_t {
    int	nused;			        /*num slots currently used in stack  */
    const char *slot[H5FS_NSLOTS];	/*array of function records	     */
} H5FS_t;

H5_DLL herr_t H5FS_push (const char *func_name);
H5_DLL herr_t H5FS_pop (void);
H5_DLL herr_t H5FS_print (FILE *stream);

#endif /* _H5FSprivate_H */
