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

H5_DLL hid_t  H5FD_srb_init(void);
H5_DLL herr_t H5Pset_fapl_srb(hid_t fapl_id, SRB_Info info);
H5_DLL herr_t H5Pget_fapl_srb(hid_t fapl_id, SRB_Info *info);

#ifdef __cplusplus
}
#endif

#else
#define H5FD_SRB   (-1)
#endif /* H5_HAVE_SRB */

#endif /* H5FDsrb_H   */
