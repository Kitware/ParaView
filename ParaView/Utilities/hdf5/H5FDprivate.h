/*
 * Copyright © 1999 NCSA
 *                  All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, July 26, 1999
 */
#ifndef _H5FDprivate_H
#define _H5FDprivate_H

#include "H5FDpublic.h"

#define H5FD_has_cmp(id) (NULL!=H5FD_get_class(id)->cmp)

__DLL__ int H5FD_term_interface(void);
__DLL__ H5FD_class_t *H5FD_get_class(hid_t id);
__DLL__ hsize_t H5FD_sb_size(H5FD_t *file);
__DLL__ herr_t H5FD_sb_encode(H5FD_t *file, char *name/*out*/, uint8_t *buf);
__DLL__ herr_t H5FD_sb_decode(H5FD_t *file, const char *name, const uint8_t *buf);
__DLL__ void *H5FD_fapl_get(H5FD_t *file);
__DLL__ void *H5FD_fapl_copy(hid_t driver_id, const void *fapl);
__DLL__ herr_t H5FD_fapl_free(hid_t driver_id, void *fapl);
__DLL__ void *H5FD_dxpl_copy(hid_t driver_id, const void *dxpl);
__DLL__ herr_t H5FD_dxpl_free(hid_t driver_id, void *dxpl);
__DLL__ H5FD_t *H5FD_open(const char *name, unsigned flags, hid_t fapl_id,
                  haddr_t maxaddr);
__DLL__ herr_t H5FD_close(H5FD_t *file);
__DLL__ int H5FD_cmp(const H5FD_t *f1, const H5FD_t *f2);
__DLL__ int H5FD_query(const H5FD_t *f, unsigned long *flags/*out*/);
__DLL__ haddr_t H5FD_alloc(H5FD_t *file, H5FD_mem_t type, hsize_t size);
__DLL__ herr_t H5FD_free(H5FD_t *file, H5FD_mem_t type, haddr_t addr, hsize_t size);
__DLL__ haddr_t H5FD_realloc(H5FD_t *file, H5FD_mem_t type, haddr_t old_addr,
                     hsize_t old_size, hsize_t new_size);
__DLL__ haddr_t H5FD_get_eoa(H5FD_t *file);
__DLL__ herr_t H5FD_set_eoa(H5FD_t *file, haddr_t addr);
__DLL__ haddr_t H5FD_get_eof(H5FD_t *file);
__DLL__ herr_t H5FD_read(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size,
                 void *buf/*out*/);
__DLL__ herr_t H5FD_write(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, haddr_t addr, hsize_t size,
                  const void *buf);
__DLL__ herr_t H5FD_flush(H5FD_t *file);

#endif /* !_H5FDprivate_H */
