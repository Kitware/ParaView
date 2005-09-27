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
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, July 26, 1999
 */
#ifndef _H5FDprivate_H
#define _H5FDprivate_H

/* Include package's public header */
#include "H5FDpublic.h"

/* Private headers needed by this file */

/*
 * The MPIO, MPIPOSIX, & FPHDF5 drivers are needed because there are
 * places where we check for things that aren't handled by these drivers.
 */
#include "H5FDfphdf5.h"
#include "H5FDmpio.h"
#include "H5FDmpiposix.h"

/* Macros */

/* Single macro to check for all file drivers that use MPI */
#define IS_H5FD_MPI(file)  \
        (IS_H5FD_MPIO(file) || IS_H5FD_MPIPOSIX(file) || IS_H5FD_FPHDF5(file))

H5_DLL int H5FD_term_interface(void);
H5_DLL H5FD_class_t *H5FD_get_class(hid_t id);
H5_DLL hsize_t H5FD_sb_size(H5FD_t *file);
H5_DLL herr_t H5FD_sb_encode(H5FD_t *file, char *name/*out*/, uint8_t *buf);
H5_DLL herr_t H5FD_sb_decode(H5FD_t *file, const char *name, const uint8_t *buf);
H5_DLL void *H5FD_fapl_get(H5FD_t *file);
H5_DLL herr_t H5FD_fapl_open(struct H5P_genplist_t *plist, hid_t driver_id, const void *driver_info);
H5_DLL herr_t H5FD_fapl_copy(hid_t driver_id, const void *fapl, void **copied_fapl);
H5_DLL herr_t H5FD_fapl_close(hid_t driver_id, void *fapl);
H5_DLL herr_t H5FD_dxpl_open(struct H5P_genplist_t *plist, hid_t driver_id, const void *driver_info);
H5_DLL herr_t H5FD_dxpl_copy(hid_t driver_id, const void *dxpl, void **copied_dxpl);
H5_DLL herr_t H5FD_dxpl_close(hid_t driver_id, void *dxpl);
H5_DLL H5FD_t *H5FD_open(const char *name, unsigned flags, hid_t fapl_id,
		  haddr_t maxaddr);
H5_DLL herr_t H5FD_close(H5FD_t *file);
H5_DLL herr_t H5FD_free_freelist(H5FD_t *file);
H5_DLL int H5FD_cmp(const H5FD_t *f1, const H5FD_t *f2);
H5_DLL int H5FD_query(const H5FD_t *f, unsigned long *flags/*out*/);
H5_DLL haddr_t H5FD_alloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);
H5_DLL herr_t H5FD_free(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size);
H5_DLL haddr_t H5FD_realloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t old_addr,
		     hsize_t old_size, hsize_t new_size);
H5_DLL haddr_t H5FD_get_eoa(H5FD_t *file);
H5_DLL herr_t H5FD_set_eoa(H5FD_t *file, haddr_t addr);
H5_DLL haddr_t H5FD_get_eof(H5FD_t *file);
H5_DLL herr_t H5FD_read(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
		 void *buf/*out*/);
H5_DLL herr_t H5FD_write(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, size_t size,
		  const void *buf);
H5_DLL herr_t H5FD_flush(H5FD_t *file, hid_t dxpl_id, unsigned closing);
H5_DLL herr_t H5FD_get_fileno(const H5FD_t *file, unsigned long *filenum);
H5_DLL herr_t H5FD_get_vfd_handle(H5FD_t *file, hid_t fapl, void** file_handle);
H5_DLL hssize_t H5FD_get_freespace(H5FD_t *file);

#endif /* !_H5FDprivate_H */
