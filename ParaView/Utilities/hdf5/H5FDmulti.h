/*
 * Copyright © 1999 NCSA
 *                  All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, August  2, 1999
 *
 * Purpose:     The public header file for the "multi" driver.
 */
#ifndef H5FDmulti_H
#define H5FDmulti_H

#include "H5Ipublic.h"

#define H5FD_MULTI      (H5FD_multi_init())

#ifdef __cplusplus
extern "C" {
#endif

__DLL__ hid_t H5FD_multi_init(void);
__DLL__ herr_t H5Pset_fapl_multi(hid_t fapl_id, const H5FD_mem_t *memb_map,
                         const hid_t *memb_fapl, const char **memb_name,
                         const haddr_t *memb_addr, hbool_t relax);
__DLL__ herr_t H5Pget_fapl_multi(hid_t fapl_id, H5FD_mem_t *memb_map/*out*/,
                         hid_t *memb_fapl/*out*/, char **memb_name/*out*/,
                         haddr_t *memb_addr/*out*/, hbool_t *relax/*out*/);
__DLL__ herr_t H5Pset_dxpl_multi(hid_t dxpl_id, const hid_t *memb_dxpl);
__DLL__ herr_t H5Pget_dxpl_multi(hid_t dxpl_id, hid_t *memb_dxpl/*out*/);

__DLL__ herr_t H5Pset_fapl_split(hid_t fapl, const char *meta_ext,
                         hid_t meta_plist_id, const char *raw_ext,
                         hid_t raw_plist_id);

#ifdef __cplusplus
}
#endif

#endif
