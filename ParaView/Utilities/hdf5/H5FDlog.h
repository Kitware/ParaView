/*
 * Copyright © 2000 NCSA
 *                  All rights reserved.
 *
 * Programmer:  Quincey Koziol <koziol@ncsa.uiuc.edu>
 *              Monday, April 17, 2000
 *
 * Purpose:     The public header file for the log driver.
 */
#ifndef H5FDlog_H
#define H5FDlog_H

#include "H5Ipublic.h"

#define H5FD_LOG        (H5FD_log_init())

#ifdef __cplusplus
extern "C" {
#endif

__DLL__ hid_t H5FD_log_init(void);
__DLL__ herr_t H5Pset_fapl_log(hid_t fapl_id, char *logfile, int verbosity);

#ifdef __cplusplus
}
#endif

#endif
