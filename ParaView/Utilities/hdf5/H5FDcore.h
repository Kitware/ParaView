/*
 * Copyright © 1999 NCSA
 *                  All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, August  2, 1999
 *
 * Purpose:     The public header file for the sec2 driver.
 */
#ifndef H5FDcore_H
#define H5FDcore_H

#include "H5Ipublic.h"

#define H5FD_CORE       (H5FD_core_init())

#ifdef __cplusplus
extern "C" {
#endif

__DLL__ hid_t H5FD_core_init(void);
__DLL__ herr_t H5Pset_fapl_core(hid_t fapl_id, size_t increment,
                                hbool_t backing_store);
__DLL__ herr_t H5Pget_fapl_core(hid_t fapl_id, size_t *increment/*out*/,
                                hbool_t *backing_store/*out*/);

#ifdef __cplusplus
}
#endif

#endif
