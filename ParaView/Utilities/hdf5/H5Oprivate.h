/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             H5Oprivate.h
 *                      Aug  5 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Object header private include file.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5Oprivate_H
#define _H5Oprivate_H

#include "H5Opublic.h"

/* Private headers needed by this file */
#include "H5private.h"
#include "H5Fprivate.h"
#include "H5Gprivate.h"
#include "H5HGprivate.h"
#include "H5Tprivate.h"
#include "H5Spublic.h"
#include "H5Zprivate.h"

/*
 * Align messages on 8-byte boundaries because we would like to copy the
 * object header chunks directly into memory and operate on them there, even
 * on 64-bit architectures.  This allows us to reduce the number of disk I/O
 * requests with a minimum amount of mem-to-mem copies.
 */
#define H5O_ALIGN(X)            (8*(((X)+8-1)/8))

#define H5O_MIN_SIZE    H5O_ALIGN(32)   /*min obj header data size           */
#define H5O_NMESGS      32              /*initial number of messages         */
#define H5O_NCHUNKS     8               /*initial number of chunks           */
#define H5O_NEW_MESG    (-1)            /*new message                        */
#define H5O_ALL         (-1)            /*delete all messages of type        */

/* Flags which are part of a message */
#define H5O_FLAG_CONSTANT       0x01u
#define H5O_FLAG_SHARED         0x02u
#define H5O_FLAG_BITS           0x03u
#define H5O_VERSION             1

/*
 * Size of object header header.
 */
#define H5O_SIZEOF_HDR(F)                                                     \
    H5O_ALIGN(1 +               /*version number        */                    \
              1 +               /*alignment             */                    \
              2 +               /*number of messages    */                    \
              4 +               /*reference count       */                    \
              4)                /*header data size      */

/*
 * Size of message header
 */
#define H5O_SIZEOF_MSGHDR(F)                                                  \
     H5O_ALIGN(2 +      /*message type          */                            \
               2 +      /*sizeof message data   */                            \
               4)       /*reserved              */

struct H5O_shared_t;
typedef struct H5O_class_t {
    int id;                              /*message type ID on disk   */
    const char  *name;                           /*for debugging             */
    size_t      native_size;                     /*size of native message    */
    void        *(*decode)(H5F_t*, const uint8_t*, struct H5O_shared_t*);
    herr_t      (*encode)(H5F_t*, uint8_t*, const void*);
    void        *(*copy)(const void*, void*);    /*copy native value         */
    size_t      (*raw_size)(H5F_t*, const void*);/*sizeof raw val            */
    herr_t      (*reset)(void *);                /*free nested data structs  */
    herr_t      (*free)(void *);                 /*free main data struct  */
    herr_t      (*get_share)(H5F_t*, const void*, struct H5O_shared_t*);
    herr_t  (*set_share)(H5F_t*, void*, const struct H5O_shared_t*);
    herr_t      (*debug)(H5F_t*, const void*, FILE*, int, int);
} H5O_class_t;

typedef struct H5O_mesg_t {
    const H5O_class_t   *type;          /*type of message                    */
    hbool_t             dirty;          /*raw out of date wrt native         */
    uint8_t             flags;          /*message flags                      */
    void                *native;        /*native format message              */
    uint8_t             *raw;           /*ptr to raw data                    */
    size_t              raw_size;       /*size with alignment                */
    int         chunkno;        /*chunk number for this mesg         */
} H5O_mesg_t;

typedef struct H5O_chunk_t {
    hbool_t     dirty;                  /*dirty flag                         */
    haddr_t     addr;                   /*chunk file address                 */
    size_t      size;                   /*chunk size                         */
    uint8_t     *image;                 /*image of file                      */
} H5O_chunk_t;

typedef struct H5O_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    hbool_t     dirty;                  /*out of data wrt disk               */
    int version;                /*version number                     */
    int nlink;                  /*link count                         */
    int nmesgs;                 /*number of messages                 */
    int alloc_nmesgs;   /*number of message slots            */
    H5O_mesg_t  *mesg;          /*array of messages                  */
    int nchunks;                /*number of chunks                   */
    int alloc_nchunks;  /*chunks allocated                   */
    H5O_chunk_t *chunk;         /*array of chunks                    */
} H5O_t;

/*
 * Null Message.
 */
#define H5O_NULL_ID     0x0000
__DLLVAR__ const H5O_class_t H5O_NULL[1];

/*
 * Simple Data Space Message.
 */
#define H5O_SDSPACE_ID  0x0001
__DLLVAR__ const H5O_class_t H5O_SDSPACE[1];

/* operates on an H5S_t struct */

/*
 * Data Type Message.
 */
#define H5O_DTYPE_ID    0x0003
__DLLVAR__ const H5O_class_t H5O_DTYPE[1];

/* operates on an H5T_t struct */

/*
 * Fill Value Message.
 */
#define H5O_FILL_ID             0x0004
__DLLVAR__ const H5O_class_t    H5O_FILL[1];

typedef struct H5O_fill_t {
    H5T_t       *type;                  /*type. Null implies same as dataset */
    size_t      size;                   /*number of bytes in the fill value  */
    void        *buf;                   /*the fill value                     */
} H5O_fill_t;


/*
 * External File List Message
 */
#define H5O_EFL_ID              0x0007  /*external file list id              */
#define H5O_EFL_ALLOC           16      /*number of slots to alloc at once   */
#define H5O_EFL_UNLIMITED       H5F_UNLIMITED /*max possible file size       */
__DLLVAR__ const H5O_class_t H5O_EFL[1];/*external file list class           */

typedef struct H5O_efl_entry_t {
    size_t      name_offset;    /*offset of name within heap         */
    char        *name;                  /*malloc'd name                      */
    off_t       offset;                 /*offset of data within file         */
    hsize_t     size;                   /*size allocated within file         */
} H5O_efl_entry_t;

typedef struct H5O_efl_t {
    haddr_t     heap_addr;              /*address of name heap               */
    int nalloc;                 /*number of slots allocated          */
    int nused;                  /*number of slots used               */
    H5O_efl_entry_t *slot;      /*array of external file entries     */
} H5O_efl_t;

/*
 * Data Layout Message.
 */
#define H5O_LAYOUT_ID           0x0008
#define H5O_LAYOUT_NDIMS        (H5S_MAX_RANK+1)
__DLLVAR__ const H5O_class_t H5O_LAYOUT[1];

typedef struct H5O_layout_t {
    int         type;                   /*type of layout, H5D_layout_t       */
    haddr_t     addr;                   /*file address of data or B-tree     */
    unsigned    ndims;                  /*num dimensions in stored data      */
    hsize_t     dim[H5O_LAYOUT_NDIMS];  /*size of data or chunk              */
} H5O_layout_t;

/*
 * Filter pipeline message.
 */
#define H5O_PLINE_ID    0x000b
__DLLVAR__ const H5O_class_t H5O_PLINE[1];

typedef struct H5O_pline_t {
    size_t      nfilters;               /*num filters defined                */
    size_t      nalloc;                 /*num elements in `filter' array     */
    struct {
        H5Z_filter_t    id;             /*filter identification number       */
        unsigned                flags;          /*defn and invocation flags          */
        char            *name;          /*optional filter name               */
        size_t          cd_nelmts;      /*number of elements in cd_values[]  */
        unsigned                *cd_values;     /*client data values                 */
    } *filter;                          /*array of filters                   */
} H5O_pline_t;

/*
 * Attribute Message.
 */
#define H5O_ATTR_ID     0x000c
__DLLVAR__ const H5O_class_t H5O_ATTR[1];

/* operates on an H5A_t struct */

/*
 * Object name message.
 */
#define H5O_NAME_ID     0x000d
__DLLVAR__ const H5O_class_t H5O_NAME[1];

typedef struct H5O_name_t {
    char        *s;                     /*ptr to malloc'd memory             */
} H5O_name_t;

/*
 * Modification time message.  The message is just a `time_t'.
 */
#define H5O_MTIME_ID    0x000e
__DLLVAR__ const H5O_class_t H5O_MTIME[1];

/*
 * Shared object message.  This message ID never really appears in an object
 * header.  Instead, bit 2 of the `Flags' field will be set and the ID field
 * will be the ID of the pointed-to message.
 */
#define H5O_SHARED_ID   0x000f
__DLLVAR__ const H5O_class_t H5O_SHARED[1];

typedef struct H5O_shared_t {
    hbool_t             in_gh;          /*shared by global heap?             */
    union {
        H5HG_t          gh;             /*global heap info                   */
        H5G_entry_t     ent;            /*symbol table entry info            */
    } u;
} H5O_shared_t;

/*
 * Object header continuation message.
 */
#define H5O_CONT_ID     0x0010
__DLLVAR__ const H5O_class_t H5O_CONT[1];

typedef struct H5O_cont_t {
    haddr_t     addr;                   /*address of continuation block      */
    size_t      size;                   /*size of continuation block         */

    /* the following field(s) do not appear on disk */
    int chunkno;                /*chunk this mesg refers to          */
} H5O_cont_t;

/*
 * Symbol table message.
 */
#define H5O_STAB_ID     0x0011
__DLLVAR__ const H5O_class_t H5O_STAB[1];

__DLL__ void *H5O_stab_fast(const H5G_cache_t *cache, const H5O_class_t *type,
                            void *_mesg);

typedef struct H5O_stab_t {
    haddr_t     btree_addr;             /*address of B-tree                  */
    haddr_t     heap_addr;              /*address of name heap               */
} H5O_stab_t;

/* General message operators */
__DLL__ herr_t H5O_create(H5F_t *f, size_t size_hint,
                          H5G_entry_t *ent/*out*/);
__DLL__ herr_t H5O_open(H5G_entry_t *ent);
__DLL__ herr_t H5O_close(H5G_entry_t *ent);
__DLL__ int H5O_link(H5G_entry_t *ent, int adjust);
__DLL__ int H5O_count(H5G_entry_t *ent, const H5O_class_t *type);
__DLL__ htri_t H5O_exists(H5G_entry_t *ent, const H5O_class_t *type,
                          int sequence);
__DLL__ void *H5O_read(H5G_entry_t *ent, const H5O_class_t *type,
                       int sequence, void *mesg);
__DLL__ int H5O_modify(H5G_entry_t *ent, const H5O_class_t *type,
                        int overwrite, unsigned flags, const void *mesg);
__DLL__ herr_t H5O_touch(H5G_entry_t *ent, hbool_t force);
__DLL__ herr_t H5O_remove(H5G_entry_t *ent, const H5O_class_t *type,
                          int sequence);
__DLL__ herr_t H5O_reset(const H5O_class_t *type, void *native);
__DLL__ void *H5O_free(const H5O_class_t *type, void *mesg);
__DLL__ void *H5O_copy(const H5O_class_t *type, const void *mesg, void *dst);
__DLL__ herr_t H5O_share(H5F_t *f, const H5O_class_t *type, const void *mesg,
                         H5HG_t *hobj/*out*/);
__DLL__ herr_t H5O_debug(H5F_t *f, haddr_t addr, FILE * stream, int indent,
                         int fwidth);

/* EFL operators */
__DLL__ hsize_t H5O_efl_total_size(H5O_efl_t *efl);
__DLL__ herr_t H5O_efl_read(H5F_t *f, const H5O_efl_t *efl, haddr_t addr,
                            hsize_t size, uint8_t *buf);
__DLL__ herr_t H5O_efl_write(H5F_t *f, const H5O_efl_t *efl, haddr_t addr,
                             hsize_t size, const uint8_t *buf);

/* Fill value operators */
__DLL__ herr_t H5O_fill_convert(H5O_fill_t *fill, H5T_t *type);

#endif
