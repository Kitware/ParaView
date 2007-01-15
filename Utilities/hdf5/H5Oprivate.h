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

/*-------------------------------------------------------------------------
 *
 * Created:    H5Oprivate.h
 *      Aug  5 1997
 *      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:    Object header private include file.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5Oprivate_H
#define _H5Oprivate_H

/* Include the public header file for this API */
#include "H5Opublic.h"          /* Object header functions                */

/* Public headers needed by this file */
#include "H5Dpublic.h"          /* Dataset functions                      */
#include "H5Spublic.h"    /* Dataspace functions        */

/* Private headers needed by this file */
#include "H5HGprivate.h"        /* Global heap functions                  */
#include "H5Tprivate.h"    /* Datatype functions        */
#include "H5Zprivate.h"         /* I/O pipeline filters        */

/* Object header macros */
#define H5O_MIN_SIZE  H5O_ALIGN(32)  /*min obj header data size       */
#define H5O_MAX_SIZE  65536          /*max obj header data size       */
#define H5O_NEW_MESG  (-1)    /*new message           */
#define H5O_ALL    (-1)    /* Operate on all messages of type   */

/* Flags which are part of a message */
#define H5O_FLAG_CONSTANT  0x01u
#define H5O_FLAG_SHARED    0x02u
#define H5O_FLAG_BITS    (H5O_FLAG_CONSTANT|H5O_FLAG_SHARED)

/* Flags for updating messages */
#define H5O_UPDATE_TIME         0x01u
#define H5O_UPDATE_DATA_ONLY    0x02u

/* Header message IDs */
#define H5O_NULL_ID  0x0000          /* Null Message.  */
#define H5O_SDSPACE_ID  0x0001          /* Simple Dataspace Message.  */
/* Complex dataspace is/was planned for message 0x0002 */
#define H5O_DTYPE_ID  0x0003          /* Datatype Message.  */
#define H5O_FILL_ID     0x0004          /* Fill Value Message. (Old)  */
#define H5O_FILL_NEW_ID 0x0005          /* Fill Value Message. (New)  */
/* Compact data storage is/was planned for message 0x0006 */
#define H5O_EFL_ID  0x0007          /* External File List Message  */
#define H5O_LAYOUT_ID  0x0008          /* Data Storage Layout Message.  */
#define H5O_BOGUS_ID  0x0009          /* "Bogus" Message.  */
/* message 0x000a appears unused... */
#define H5O_PLINE_ID  0x000b          /* Filter pipeline message.  */
#define H5O_ATTR_ID  0x000c          /* Attribute Message.  */
#define H5O_NAME_ID  0x000d          /* Object name message.  */
#define H5O_MTIME_ID  0x000e          /* Modification time message. (Old)  */
#define H5O_SHARED_ID  0x000f          /* Shared object message.  */
#define H5O_CONT_ID  0x0010          /* Object header continuation message.  */
#define H5O_STAB_ID  0x0011          /* Symbol table message.  */
#define H5O_MTIME_NEW_ID 0x0012         /* Modification time message. (New)  */

/*
 * Fill Value Message. (Old)
 * (Data structure in memory)
 */
typedef struct H5O_fill_t {
    H5T_t               *type;          /*type. Null implies same as dataset */
    size_t              size;           /*number of bytes in the fill value  */
    void                *buf;           /*the fill value                     */
} H5O_fill_t;

/*
 * New Fill Value Message.  The new fill value message is fill value plus
 * space allocation time and fill value writing time and whether fill
 * value is defined.
 */

typedef struct H5O_fill_new_t {
    H5T_t    *type;    /*type. Null implies same as dataset */
    ssize_t    size;    /*number of bytes in the fill value  */
    void    *buf;    /*the fill value         */
    H5D_alloc_time_t  alloc_time;  /* time to allocate space       */
    H5D_fill_time_t  fill_time;  /* time to write fill value       */
    hbool_t    fill_defined;   /* whether fill value is defined     */
} H5O_fill_new_t;

/*
 * External File List Message
 * (Data structure in memory)
 */
#define H5O_EFL_ALLOC    16  /*number of slots to alloc at once   */
#define H5O_EFL_UNLIMITED  H5F_UNLIMITED /*max possible file size       */

typedef struct H5O_efl_entry_t {
    size_t  name_offset;    /*offset of name within heap       */
    char  *name;      /*malloc'd name           */
    off_t  offset;      /*offset of data within file       */
    hsize_t  size;      /*size allocated within file       */
} H5O_efl_entry_t;

typedef struct H5O_efl_t {
    haddr_t  heap_addr;    /*address of name heap         */
    size_t  nalloc;      /*number of slots allocated       */
    size_t  nused;      /*number of slots used         */
    H5O_efl_entry_t *slot;    /*array of external file entries     */
} H5O_efl_t;

/*
 * Data Layout Message.
 * (Data structure in memory)
 */
#define H5O_LAYOUT_NDIMS  (H5S_MAX_RANK+1)

typedef struct H5O_layout_contig_t {
    haddr_t  addr;      /* File address of data              */
    hsize_t     size;                   /* Size of data in bytes             */
} H5O_layout_contig_t;

typedef struct H5O_layout_chunk_t {
    haddr_t  addr;      /* File address of B-tree            */
    unsigned  ndims;      /* Num dimensions in chunk           */
    size_t  dim[H5O_LAYOUT_NDIMS];  /* Size of chunk in elements         */
    size_t      size;                   /* Size of chunk in bytes            */
    H5RC_t     *btree_shared;           /* Ref-counted info for B-tree nodes */
} H5O_layout_chunk_t;

typedef struct H5O_layout_compact_t {
    hbool_t     dirty;                  /* Dirty flag for compact dataset    */
    size_t      size;                   /* Size of buffer in bytes           */
    void        *buf;                   /* Buffer for compact dataset        */
} H5O_layout_compact_t;

typedef struct H5O_layout_t {
    H5D_layout_t type;      /* Type of layout                    */
    unsigned version;                   /* Version of message                */
    /* Structure for "unused" dimension information */
    struct {
        unsigned ndims;      /*num dimensions in stored data       */
        hsize_t  dim[H5O_LAYOUT_NDIMS];  /*size of data or chunk in bytes     */
    } unused;
    union {
        H5O_layout_contig_t contig;     /* Information for contiguous layout */
        H5O_layout_chunk_t chunk;       /* Information for chunked layout    */
        H5O_layout_compact_t compact;   /* Information for compact layout    */
    } u;
} H5O_layout_t;

/* Enable reading/writing "bogus" messages */
/* #define H5O_ENABLE_BOGUS */

#ifdef H5O_ENABLE_BOGUS
/*
 * "Bogus" Message.
 * (Data structure in memory)
 */
#define H5O_BOGUS_VALUE         0xdeadbeef
typedef struct H5O_bogus_t {
    unsigned u;                         /* Hold the bogus info */
} H5O_bogus_t;
#endif /* H5O_ENABLE_BOGUS */

/*
 * Filter pipeline message.
 * (Data structure in memory)
 */
typedef struct H5O_pline_t {
    size_t  nalloc;      /*num elements in `filter' array     */
    size_t  nused;      /*num filters defined         */
    H5Z_filter_info_t *filter;    /*array of filters         */
} H5O_pline_t;

/*
 * Object name message.
 * (Data structure in memory)
 */

typedef struct H5O_name_t {
    char  *s;      /*ptr to malloc'd memory       */
} H5O_name_t;

/*
 * Shared object message.  This message ID never really appears in an object
 * header.  Instead, bit 2 of the `Flags' field will be set and the ID field
 * will be the ID of the pointed-to message.
 */

typedef struct H5O_shared_t {
    hbool_t    in_gh;    /*shared by global heap?       */
    union {
  H5HG_t    gh;    /*global heap info         */
  H5G_entry_t  ent;    /*symbol table entry info       */
    } u;
} H5O_shared_t;

/*
 * Object header continuation message.
 * (Data structure in memory)
 */

typedef struct H5O_cont_t {
    haddr_t  addr;      /*address of continuation block       */
    size_t  size;      /*size of continuation block       */

    /* the following field(s) do not appear on disk */
    unsigned  chunkno;    /*chunk this mesg refers to       */
} H5O_cont_t;

/*
 * Symbol table message.
 * (Data structure in memory)
 */
typedef struct H5O_stab_t {
    haddr_t  btree_addr;    /*address of B-tree         */
    haddr_t  heap_addr;    /*address of name heap         */
} H5O_stab_t;

/* Define return values from operator callback function for H5O_iterate */
/* (Actually, any postive value will cause the iterator to stop and pass back
 *      that positive value to the function that called the iterator)
 */
#define H5O_ITER_ERROR  (-1)
#define H5O_ITER_CONT   (0)
#define H5O_ITER_STOP   (1)

/* Typedef for iteration operations */
typedef herr_t (*H5O_operator_t)(const void *mesg/*in*/, unsigned idx,
    void *operator_data/*in,out*/);

/* General message operators */
H5_DLL herr_t H5O_create(H5F_t *f, hid_t dxpl_id, size_t size_hint,
        H5G_entry_t *ent/*out*/);
H5_DLL herr_t H5O_open(const H5G_entry_t *ent);
H5_DLL herr_t H5O_close(H5G_entry_t *ent);
H5_DLL int H5O_link(const H5G_entry_t *ent, int adjust, hid_t dxpl_id);
H5_DLL int H5O_count(H5G_entry_t *ent, unsigned type_id, hid_t dxpl_id);
H5_DLL htri_t H5O_exists(H5G_entry_t *ent, unsigned type_id, int sequence,
    hid_t dxpl_id);
H5_DLL void *H5O_read(const H5G_entry_t *ent, unsigned type_id, int sequence,
    void *mesg, hid_t dxpl_id);
H5_DLL int H5O_modify(H5G_entry_t *ent, unsigned type_id,
    int overwrite, unsigned flags, unsigned update_flags, const void *mesg, hid_t dxpl_id);
H5_DLL struct H5O_t * H5O_protect(H5G_entry_t *ent, hid_t dxpl_id);
H5_DLL herr_t H5O_unprotect(H5G_entry_t *ent, struct H5O_t *oh, hid_t dxpl_id);
H5_DLL int H5O_append(H5F_t *f, hid_t dxpl_id, struct H5O_t *oh, unsigned type_id,
    unsigned flags, const void *mesg);
H5_DLL herr_t H5O_touch(H5G_entry_t *ent, hbool_t force, hid_t dxpl_id);
H5_DLL herr_t H5O_touch_oh(H5F_t *f, struct H5O_t *oh, hbool_t force);
#ifdef H5O_ENABLE_BOGUS
H5_DLL herr_t H5O_bogus(H5G_entry_t *ent, hid_t dxpl_id);
H5_DLL herr_t H5O_bogus_oh(H5F_t *f, struct H5O_t *oh);
#endif /* H5O_ENABLE_BOGUS */
H5_DLL herr_t H5O_remove(H5G_entry_t *ent, unsigned type_id, int sequence,
    hbool_t adj_link, hid_t dxpl_id);
H5_DLL herr_t H5O_remove_op(H5G_entry_t *ent, unsigned type_id,
    H5O_operator_t op, void *op_data, hbool_t adj_link, hid_t dxpl_id);
H5_DLL herr_t H5O_reset(unsigned type_id, void *native);
H5_DLL void *H5O_free(unsigned type_id, void *mesg);
H5_DLL void *H5O_copy(unsigned type_id, const void *mesg, void *dst);
H5_DLL size_t H5O_raw_size(unsigned type_id, const H5F_t *f, const void *mesg);
H5_DLL size_t H5O_mesg_size(unsigned type_id, const H5F_t *f, const void *mesg);
H5_DLL herr_t H5O_get_share(unsigned type_id, H5F_t *f, const void *mesg, H5O_shared_t *share);
H5_DLL herr_t H5O_delete(H5F_t *f, hid_t dxpl_id, haddr_t addr);
H5_DLL herr_t H5O_get_info(H5G_entry_t *ent, H5O_stat_t *ostat, hid_t dxpl_id);
H5_DLL herr_t H5O_iterate(const H5G_entry_t *ent, unsigned type_id, H5O_operator_t op,
    void *op_data, hid_t dxpl_id);
H5_DLL herr_t H5O_debug_id(hid_t type_id, H5F_t *f, hid_t dxpl_id, const void *mesg, FILE *stream, int indent, int fwidth);
H5_DLL herr_t H5O_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE * stream, int indent,
       int fwidth);

/* Layout operators */
H5_DLL size_t H5O_layout_meta_size(const H5F_t *f, const void *_mesg);

/* EFL operators */
H5_DLL hsize_t H5O_efl_total_size(H5O_efl_t *efl);

/* Fill value operators */
H5_DLL herr_t H5O_fill_convert(void *_fill, H5T_t *type, hid_t dxpl_id);

#endif
