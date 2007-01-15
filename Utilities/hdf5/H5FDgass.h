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
 * Programmer:  Saurabh Bagchi <bagchi@uiuc.edu>
 *              Tuesday, August 17, 1999
 *
 * Purpose:  The public header file for the gass driver.
 */
#ifndef H5FDgass_H
#define H5FDgass_H

#include "H5FDpublic.h"
#include "H5Ipublic.h"

#include <string.h>

#ifdef H5_HAVE_GASS
#define H5FD_GASS        (H5FD_gass_init())
#else
#define H5FD_GASS        (-1)
#endif

#ifdef H5_HAVE_GASS
/* Define the GASS info object. (Will be added to later as more GASS
   functionality is sought to be exposed. */
typedef struct GASS_Info {
  unsigned long block_size;
  unsigned long max_length;
} GASS_Info;

#define GASS_INFO_NULL(v) memset((void *)&v, 0, sizeof(GASS_Info));
/*
  GASS_Info zzGassInfo = {0L,0L};
  #define GASS_INFO_NULL zzGassInfo
*/
#endif

/* Function prototypes */
#ifdef H5_HAVE_GASS

#ifdef __cplusplus
extern "C" {
#endif

hid_t H5FD_gass_init(void);
void H5FD_gass_term(void);
herr_t H5Pset_fapl_gass(hid_t fapl_id, GASS_Info info);
herr_t H5Pget_fapl_gass(hid_t fapl_id, GASS_Info *info/*out*/);

#ifdef __cplusplus
}
#endif

#endif

#endif /* H5FDgass_H */

