/*
 * Copyright © 1999 NCSA
 *                  All rights reserved.
 *
 * Programmer:  Thomas Radke <tradke@aei-potsdam.mpg.de>
 *              Monday, October 11, 1999
 *
 * Purpose:     The public header file for the DPSS driver.
 */
#ifndef H5FDdpss_H
#define H5FDdpss_H

#include "H5public.h"          /* typedef for herr_t */
#include "H5Ipublic.h"         /* typedef for hid_t  */

#ifdef H5_HAVE_GRIDSTORAGE 
#define H5FD_DPSS              (H5FD_dpss_init())
#else
#define H5FD_DPSS              (-1)
#endif   

/* Function prototypes */
#ifdef H5_HAVE_GRIDSTORAGE

#ifdef __cplusplus
extern "C" {
#endif

hid_t H5FD_dpss_init(void);
herr_t H5Pset_fapl_dpss (hid_t fapl_id);
herr_t H5Pget_fapl_dpss (hid_t fapl_id);

#ifdef __cplusplus
}
#endif

#endif

#endif /* H5FDdpss_H */
