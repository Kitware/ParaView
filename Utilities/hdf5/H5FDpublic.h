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
#ifndef _H5FDpublic_H
#define _H5FDpublic_H

#include "H5public.h"
#include "H5Fpublic.h"		/*for H5F_close_degree_t */

#define H5_HAVE_VFL 1 /*define a convenient app feature test*/
#define H5FD_VFD_DEFAULT 0   /* Default VFL driver value */

/*
 * Types of allocation requests. The values larger than H5FD_MEM_DEFAULT
 * should not change other than adding new types to the end. These numbers
 * might appear in files.
 */
typedef enum H5FD_mem_t {
    H5FD_MEM_NOLIST	= -1,			/*must be negative*/
    H5FD_MEM_DEFAULT	= 0,			/*must be zero*/
    H5FD_MEM_SUPER      = 1,
    H5FD_MEM_BTREE      = 2,
    H5FD_MEM_DRAW       = 3,
    H5FD_MEM_GHEAP      = 4,
    H5FD_MEM_LHEAP      = 5,
    H5FD_MEM_OHDR       = 6,

    H5FD_MEM_NTYPES				/*must be last*/
} H5FD_mem_t;

/*
 * A free-list map which maps all types of allocation requests to a single
 * free list.  This is useful for drivers that don't really care about
 * keeping different requests segregated in the underlying file and which
 * want to make most efficient reuse of freed memory.  The use of the
 * H5FD_MEM_SUPER free list is arbitrary.
 */
#define H5FD_FLMAP_SINGLE {						      \
    H5FD_MEM_SUPER,			/*default*/			      \
    H5FD_MEM_SUPER,			/*super*/			      \
    H5FD_MEM_SUPER,			/*btree*/			      \
    H5FD_MEM_SUPER,			/*draw*/			      \
    H5FD_MEM_SUPER,			/*gheap*/			      \
    H5FD_MEM_SUPER,			/*lheap*/			      \
    H5FD_MEM_SUPER			/*ohdr*/			      \
}

/*
 * A free-list map which segregates requests into `raw' or `meta' data
 * pools.
 */
#define H5FD_FLMAP_DICHOTOMY {						      \
    H5FD_MEM_SUPER,			/*default*/			      \
    H5FD_MEM_SUPER,			/*super*/			      \
    H5FD_MEM_SUPER,			/*btree*/			      \
    H5FD_MEM_DRAW,			/*draw*/			      \
    H5FD_MEM_SUPER,			/*gheap*/			      \
    H5FD_MEM_SUPER,			/*lheap*/			      \
    H5FD_MEM_SUPER			/*ohdr*/			      \
}

/*
 * The default free list map which causes each request type to use it's own
 * free-list.
 */
#define H5FD_FLMAP_DEFAULT {						      \
    H5FD_MEM_DEFAULT,			/*default*/			      \
    H5FD_MEM_DEFAULT,			/*super*/			      \
    H5FD_MEM_DEFAULT,			/*btree*/			      \
    H5FD_MEM_DEFAULT,			/*draw*/			      \
    H5FD_MEM_DEFAULT,			/*gheap*/			      \
    H5FD_MEM_DEFAULT,			/*lheap*/			      \
    H5FD_MEM_DEFAULT			/*ohdr*/			      \
}


/* Define VFL driver features that can be enabled on a per-driver basis */
/* These are returned with the 'query' function pointer in H5FD_class_t */
    /*
     * Defining the H5FD_FEAT_AGGREGATE_METADATA for a VFL driver means that
     * the library will attempt to allocate a larger block for metadata and
     * then sub-allocate each metadata request from that larger block.
     */
#define H5FD_FEAT_AGGREGATE_METADATA    0x00000001
    /*
     * Defining the H5FD_FEAT_ACCUMULATE_METADATA for a VFL driver means that
     * the library will attempt to cache metadata as it is written to the file
     * and build up a larger block of metadata to eventually pass to the VFL
     * 'write' routine.
     * 
     * Distinguish between updating the metadata accumulator on writes and
     * reads.  This is particularly (perhaps only, even) important for MPI-I/O
     * where we guarantee that writes are collective, but reads may not be.
     * If we were to allow the metadata accumulator to be written during a
     * read operation, the application would hang.
     */
#define H5FD_FEAT_ACCUMULATE_METADATA_WRITE     0x00000002
#define H5FD_FEAT_ACCUMULATE_METADATA_READ      0x00000004
#define H5FD_FEAT_ACCUMULATE_METADATA   (H5FD_FEAT_ACCUMULATE_METADATA_WRITE|H5FD_FEAT_ACCUMULATE_METADATA_READ)
    /*
     * Defining the H5FD_FEAT_DATA_SIEVE for a VFL driver means that
     * the library will attempt to cache raw data as it is read from/written to
     * a file in a "data seive" buffer.  See Rajeev Thakur's papers:
     *  http://www.mcs.anl.gov/~thakur/papers/romio-coll.ps.gz
     *  http://www.mcs.anl.gov/~thakur/papers/mpio-high-perf.ps.gz
     */
#define H5FD_FEAT_DATA_SIEVE            0x00000008
    /*
     * Defining the H5FD_FEAT_AGGREGATE_SMALLDATA for a VFL driver means that
     * the library will attempt to allocate a larger block for "small" raw data
     * and then sub-allocate "small" raw data requests from that larger block.
     */
#define H5FD_FEAT_AGGREGATE_SMALLDATA   0x00000010
	

/* Forward declaration */
typedef struct H5FD_t H5FD_t;

/* Class information for each file driver */
typedef struct H5FD_class_t {
    const char *name;
    haddr_t maxaddr;
    H5F_close_degree_t fc_degree;
    hsize_t (*sb_size)(H5FD_t *file);
    herr_t  (*sb_encode)(H5FD_t *file, char *name/*out*/,
                         unsigned char *p/*out*/);
    herr_t  (*sb_decode)(H5FD_t *f, const char *name, const unsigned char *p);
    size_t  fapl_size;
    void *  (*fapl_get)(H5FD_t *file);
    void *  (*fapl_copy)(const void *fapl);
    herr_t  (*fapl_free)(void *fapl);
    size_t  dxpl_size;
    void *  (*dxpl_copy)(const void *dxpl);
    herr_t  (*dxpl_free)(void *dxpl);
    H5FD_t *(*open)(const char *name, unsigned flags, hid_t fapl,
                    haddr_t maxaddr);
    herr_t  (*close)(H5FD_t *file);
    int     (*cmp)(const H5FD_t *f1, const H5FD_t *f2);
    herr_t  (*query)(const H5FD_t *f1, unsigned long *flags);
    haddr_t (*alloc)(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);
    herr_t  (*free)(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                    haddr_t addr, hsize_t size);
    haddr_t (*get_eoa)(H5FD_t *file);
    herr_t  (*set_eoa)(H5FD_t *file, haddr_t addr);
    haddr_t (*get_eof)(H5FD_t *file);
    herr_t  (*get_handle)(H5FD_t *file, hid_t fapl, void**file_handle);
    herr_t  (*read)(H5FD_t *file, H5FD_mem_t type, hid_t dxpl,
                    haddr_t addr, size_t size, void *buffer);
    herr_t  (*write)(H5FD_t *file, H5FD_mem_t type, hid_t dxpl,
                     haddr_t addr, size_t size, const void *buffer);
    herr_t  (*flush)(H5FD_t *file, hid_t dxpl_id, unsigned closing);
    herr_t  (*lock)(H5FD_t *file, unsigned char *oid, unsigned lock_type, hbool_t last);
    herr_t  (*unlock)(H5FD_t *file, unsigned char *oid, hbool_t last);
    H5FD_mem_t fl_map[H5FD_MEM_NTYPES];
} H5FD_class_t;

/* A free list is a singly-linked list of address/size pairs. */
typedef struct H5FD_free_t {
    haddr_t		addr;
    hsize_t		size;
    struct H5FD_free_t	*next;
} H5FD_free_t;

/*
 * The main datatype for each driver. Public fields common to all drivers
 * are declared here and the driver appends private fields in memory.
 */
struct H5FD_t {
    hid_t               driver_id;      /*driver ID for this file   */
    const H5FD_class_t *cls;            /*constant class info       */
    unsigned long       fileno[2];      /* File serial number       */
    unsigned long       feature_flags;  /* VFL Driver feature Flags */
    hsize_t             threshold;      /* Threshold for alignment  */
    hsize_t             alignment;      /* Allocation alignment     */

    /* Metadata aggregation fields */
    hsize_t             def_meta_block_size;  /* Metadata allocation
                                               * block size (if
                                               * aggregating metadata) */
    hsize_t             cur_meta_block_size;  /* Current size of metadata
                                               * allocation region left */
    haddr_t             eoma;                 /* End of metadata
                                               * allocated region */

    /* "Small data" aggregation fields */
    hsize_t             def_sdata_block_size;   /* "Small data"
                                                 * allocation block size
                                                 * (if aggregating "small
                                                 * data") */
    hsize_t             cur_sdata_block_size;   /* Current size of "small
                                                 * data" allocation
                                                 * region left */
    haddr_t             eosda;                  /* End of "small data"
                                                 * allocated region */

    /* Metadata accumulator fields */
    unsigned char      *meta_accum;     /* Buffer to hold the accumulated metadata */
    haddr_t             accum_loc;      /* File location (offset) of the
                                         * accumulated metadata */
    size_t              accum_size;     /* Size of the accumulated
                                         * metadata buffer used (in
                                         * bytes) */
    size_t              accum_buf_size; /* Size of the accumulated
                                         * metadata buffer allocated (in
                                         * bytes) */
    unsigned            accum_dirty;    /* Flag to indicate that the
                                         * accumulated metadata is dirty */
    haddr_t             maxaddr;        /* For this file, overrides class */
    H5FD_free_t        *fl[H5FD_MEM_NTYPES]; /* Freelist per allocation type */
    hsize_t             maxsize;        /* Largest object on FL, or zero */
};

#ifdef __cplusplus
extern "C" {
#endif

/* Function prototypes */
H5_DLL hid_t H5FDregister(const H5FD_class_t *cls);
H5_DLL herr_t H5FDunregister(hid_t driver_id);
H5_DLL H5FD_t *H5FDopen(const char *name, unsigned flags, hid_t fapl_id,
                        haddr_t maxaddr);
H5_DLL herr_t H5FDclose(H5FD_t *file);
H5_DLL int H5FDcmp(const H5FD_t *f1, const H5FD_t *f2);
H5_DLL int H5FDquery(const H5FD_t *f, unsigned long *flags);
H5_DLL haddr_t H5FDalloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id, hsize_t size);
H5_DLL herr_t H5FDfree(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                       haddr_t addr, hsize_t size);
H5_DLL haddr_t H5FDrealloc(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                           haddr_t addr, hsize_t old_size, hsize_t new_size);
H5_DLL haddr_t H5FDget_eoa(H5FD_t *file);
H5_DLL herr_t H5FDset_eoa(H5FD_t *file, haddr_t eof);
H5_DLL haddr_t H5FDget_eof(H5FD_t *file);
H5_DLL herr_t H5FDget_vfd_handle(H5FD_t *file, hid_t fapl, void**file_handle);
H5_DLL herr_t H5FDread(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                       haddr_t addr, size_t size, void *buf/*out*/);
H5_DLL herr_t H5FDwrite(H5FD_t *file, H5FD_mem_t type, hid_t dxpl_id,
                        haddr_t addr, size_t size, const void *buf);
H5_DLL herr_t H5FDflush(H5FD_t *file, hid_t dxpl_id, unsigned closing);

#ifdef __cplusplus
}
#endif
#endif
