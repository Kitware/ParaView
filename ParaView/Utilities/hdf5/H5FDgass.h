/*
 * Copyright © 1999 NCSA
 *                  All rights reserved.
 *
 * Programmer:  Saurabh Bagchi <bagchi@uiuc.edu>
 *              Tuesday, August 17, 1999
 *
 * Purpose:     The public header file for the gass driver.
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
herr_t H5Pset_fapl_gass(hid_t fapl_id, GASS_Info info);
herr_t H5Pget_fapl_gass(hid_t fapl_id, GASS_Info *info/*out*/);

#ifdef __cplusplus
}
#endif

#endif

#endif /* H5FDgass_H */

