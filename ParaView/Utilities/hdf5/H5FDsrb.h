/*
 * Copyright (c) 1999 NCSA
 *               All rights reserved.
 *
 * Programmer: Raymond Lu <slu@ncsa.uiuc.edu>
 *             Wednesday, April 12, 2000
 * Purpose:    The public header file for the SRB driver.
 */
#ifndef H5FDsrb_H
#define H5FDsrb_H

#include "H5FDpublic.h"
#include "H5Ipublic.h"

#ifdef H5_HAVE_SRB

#define H5FD_SRB   (H5FD_srb_init())

typedef struct SRB_Info {    /* Define the SRB info object.                  */
    char *srbHost;           /* SRB host address of server                   */
    char *srbPort;           /* SRB host port number                         */
    char *srbAuth;           /* SRB Authentication-password                  */
    int  storSysType;        /* Storage Type: 0=Unix, 1=UniTree, 2=HPSS,
                              *               3=FTP, 4=HTTP                  */
    int  mode;               /* File mode-Unix access mode                   */
    int  size;               /* File Size-Only valid for HPSS, -1 is default */
} SRB_Info;

#ifdef __cplusplus
extern "C" {
#endif

__DLL__ hid_t  H5FD_srb_init(void);
__DLL__ herr_t H5Pset_fapl_srb(hid_t fapl_id, SRB_Info info);
__DLL__ herr_t H5Pget_fapl_srb(hid_t fapl_id, SRB_Info *info);

#ifdef __cplusplus
}
#endif

#else
#define H5FD_SRB   (-1)
#endif /* H5_HAVE_SRB */

#endif /* H5FDsrb_H   */
