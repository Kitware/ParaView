/*
 * Copyright © 1999 NCSA
 *                  All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, August  2, 1999
 *
 * Purpose:     The public header file for the sec2 driver.
 */
#ifndef H5FDsec2_H
#define H5FDsec2_H

#include "H5Ipublic.h"

#define H5FD_SEC2       (H5FD_sec2_init())

#ifdef __cplusplus
extern "C" {
#endif

__DLL__ hid_t H5FD_sec2_init(void);
__DLL__ herr_t H5Pset_fapl_sec2(hid_t fapl_id);

#ifdef __cplusplus
}
#endif

#endif
