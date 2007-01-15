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
 * This file contains macros & information for file access
 */

#ifndef _H5Fprivate_H
#define _H5Fprivate_H

/* Include package's public header */
#include "H5Fpublic.h"

/* Public headers needed by this file */
#include "H5FDpublic.h"    /* File drivers        */

/* Private headers needed by this file */

/****************************/
/* Library Private Typedefs */
/****************************/

/* Main file structure */
typedef struct H5F_t H5F_t;

/*===----------------------------------------------------------------------===
 *                              Flush Flags
 *===----------------------------------------------------------------------===
 *
 *  Flags passed into the flush routines which indicate what type of
 *  flush we want to do. They can be ORed together.
 */
#define H5F_FLUSH_NONE       (0U)       /* No flags specified                       */
#define H5F_FLUSH_INVALIDATE (1U << 0)  /* Invalidate cached data                   */
#define H5F_FLUSH_CLOSING    (1U << 1)  /* Closing the file                         */
#define H5F_FLUSH_CLEAR_ONLY (1U << 2)  /* Don't write, just clear dirty flags      */

/*
 * Encode and decode macros for file meta-data.
 * Currently, all file meta-data is little-endian.
 */

#  define INT16ENCODE(p, i) {                  \
   *(p) = (uint8_t)( (unsigned)(i)   & 0xff); (p)++;          \
   *(p) = (uint8_t)(((unsigned)(i) >> 8) & 0xff); (p)++;          \
}

#  define UINT16ENCODE(p, i) {                  \
   *(p) = (uint8_t)( (unsigned)(i)   & 0xff); (p)++;          \
   *(p) = (uint8_t)(((unsigned)(i) >> 8) & 0xff); (p)++;          \
}

#  define INT32ENCODE(p, i) {                  \
   *(p) = (uint8_t)( (uint32_t)(i)    & 0xff); (p)++;          \
   *(p) = (uint8_t)(((uint32_t)(i) >>  8) & 0xff); (p)++;          \
   *(p) = (uint8_t)(((uint32_t)(i) >> 16) & 0xff); (p)++;          \
   *(p) = (uint8_t)(((uint32_t)(i) >> 24) & 0xff); (p)++;          \
}

#  define UINT32ENCODE(p, i) {                  \
   *(p) = (uint8_t)( (i)        & 0xff); (p)++;              \
   *(p) = (uint8_t)(((i) >>  8) & 0xff); (p)++;              \
   *(p) = (uint8_t)(((i) >> 16) & 0xff); (p)++;              \
   *(p) = (uint8_t)(((i) >> 24) & 0xff); (p)++;              \
}

#  define INT64ENCODE(p, n) {                  \
   int64_t _n = (n);                    \
   size_t _i;                      \
   uint8_t *_p = (uint8_t*)(p);                  \
   for (_i=0; _i<sizeof(int64_t); _i++, _n>>=8) {            \
      *_p++ = (uint8_t)(_n & 0xff);                \
   }                        \
   for (/*void*/; _i<8; _i++) {                  \
      *_p++ = (n)<0 ? 0xff : 0;                  \
   }                        \
   (p) = (uint8_t*)(p)+8;                  \
}

#  define UINT64ENCODE(p, n) {                  \
   uint64_t _n = (n);                    \
   size_t _i;                      \
   uint8_t *_p = (uint8_t*)(p);                  \
   for (_i=0; _i<sizeof(uint64_t); _i++, _n>>=8) {            \
      *_p++ = (uint8_t)(_n & 0xff);                \
   }                        \
   for (/*void*/; _i<8; _i++) {                  \
      *_p++ = 0;                    \
   }                        \
   (p) = (uint8_t*)(p)+8;                  \
}

/* DECODE converts little endian bytes pointed by p to integer values and store
 * it in i.  For signed values, need to do sign-extension when converting
 * the last byte which carries the sign bit.
 * The macros does not require i be of a certain byte sizes.  It just requires
 * i be big enough to hold the intended value range.  E.g. INT16DECODE works
 * correctly even if i is actually a 64bit int like in a Cray.
 */

#  define INT16DECODE(p, i) {                  \
   (i)  = (int16_t)((*(p) & 0xff));      (p)++;              \
   (i) |= (int16_t)(((*(p) & 0xff) << 8) |                                    \
                   ((*(p) & 0x80) ? ~0xffff : 0x0)); (p)++;          \
}

#  define UINT16DECODE(p, i) {                  \
   (i)  = (uint16_t) (*(p) & 0xff);    (p)++;            \
   (i) |= (uint16_t)((*(p) & 0xff) << 8); (p)++;            \
}

#  define INT32DECODE(p, i) {                  \
   (i)  = (       *(p) & 0xff);    (p)++;            \
   (i) |= ((int32_t)(*(p) & 0xff) <<  8); (p)++;            \
   (i) |= ((int32_t)(*(p) & 0xff) << 16); (p)++;            \
   (i) |= ((int32_t)(((*(p) & 0xff) << 24) |                                  \
                   ((*(p) & 0x80) ? ~0xffffffff : 0x0))); (p)++;        \
}

#  define UINT32DECODE(p, i) {                  \
   (i)  =  (uint32_t)(*(p) & 0xff);     (p)++;            \
   (i) |= ((uint32_t)(*(p) & 0xff) <<  8); (p)++;            \
   (i) |= ((uint32_t)(*(p) & 0xff) << 16); (p)++;            \
   (i) |= ((uint32_t)(*(p) & 0xff) << 24); (p)++;            \
}

#  define INT64DECODE(p, n) {                  \
   /* WE DON'T CHECK FOR OVERFLOW! */                \
   size_t _i;                      \
   n = 0;                      \
   (p) += 8;                      \
   for (_i=0; _i<sizeof(int64_t); _i++) {              \
      n = (n<<8) | *(--p);                  \
   }                        \
   (p) += 8;                      \
}

#  define UINT64DECODE(p, n) {                  \
   /* WE DON'T CHECK FOR OVERFLOW! */                \
   size_t _i;                      \
   n = 0;                      \
   (p) += 8;                      \
   for (_i=0; _i<sizeof(uint64_t); _i++) {              \
      n = (n<<8) | *(--p);                  \
   }                        \
   (p) += 8;                      \
}

#define NBYTEENCODE(d, s, n)    do { HDmemcpy(d,s,n); d += n; } while (0)

/*
 * Note:  the NBYTEDECODE macro is backwards from the memcpy() routine, in
 *    the spirit of the other DECODE macros.
 */
#define NBYTEDECODE(s, d, n)    do { HDmemcpy(d,s,n); s += n; } while (0)

/* Address-related macros */
#define H5F_addr_overflow(X,Z)  (HADDR_UNDEF==(X) ||            \
         HADDR_UNDEF==(X)+(haddr_t)(Z) ||        \
         (X)+(haddr_t)(Z)<(X))
#define H5F_addr_hash(X,M)  ((unsigned)((X)%(M)))
#define H5F_addr_defined(X)  (X!=HADDR_UNDEF)
/* The H5F_addr_eq() macro guarantees that Y is not HADDR_UNDEF by making
 * certain that X is not HADDR_UNDEF and then checking that X equals Y
 */
#define H5F_addr_eq(X,Y)  ((X)!=HADDR_UNDEF &&            \
         (X)==(Y))
#define H5F_addr_ne(X,Y)  (!H5F_addr_eq((X),(Y)))
#define H5F_addr_lt(X,Y)   ((X)!=HADDR_UNDEF &&            \
         (Y)!=HADDR_UNDEF &&            \
         (X)<(Y))
#define H5F_addr_le(X,Y)  ((X)!=HADDR_UNDEF &&            \
         (Y)!=HADDR_UNDEF &&            \
         (X)<=(Y))
#define H5F_addr_gt(X,Y)  ((X)!=HADDR_UNDEF &&            \
         (Y)!=HADDR_UNDEF &&            \
         (X)>(Y))
#define H5F_addr_ge(X,Y)  ((X)!=HADDR_UNDEF &&            \
         (Y)!=HADDR_UNDEF &&            \
         (X)>=(Y))
#define H5F_addr_cmp(X,Y)  (H5F_addr_eq(X,Y)?0:            \
         (H5F_addr_lt(X, Y)?-1:1))
#define H5F_addr_pow2(N)  ((haddr_t)1<<(N))
#define H5F_addr_overlap(O1,L1,O2,L2) ((O1<O2 && (O1+L1)>O2) ||               \
                                 (O1>=O2 && O1<(O2+L2)))

/* If the module using this macro is allowed access to the private variables, access them directly */
#ifdef H5F_PACKAGE
/* size of size_t and off_t as they exist on disk */
#define H5F_SIZEOF_ADDR(F)      ((F)->shared->sizeof_addr)
#define H5F_SIZEOF_SIZE(F)      ((F)->shared->sizeof_size)
/* Size of symbol table leafs */
#define H5F_SYM_LEAF_K(F)       ((F)->shared->sym_leaf_k)
/* B-tree key value size */
#define H5F_KVALUE(F,T)         ((F)->shared->btree_k[(T)->id])
/* Raw data cache values */
#define H5F_RDCC_NELMTS(F)      ((F)->shared->rdcc_nelmts)
#define H5F_RDCC_NBYTES(F)      ((F)->shared->rdcc_nbytes)
#define H5F_RDCC_W0(F)          ((F)->shared->rdcc_w0)
/* Check for file driver feature enabled */
#define H5F_HAS_FEATURE(F,FL)   ((F)->shared->lf->feature_flags&(FL))
/* B-tree node raw page */
#define H5F_GRP_BTREE_SHARED(F) ((F)->shared->grp_btree_shared)
/* Base address of file */
#define H5F_BASE_ADDR(F)        ((F)->shared->base_addr)
/* Sieve buffer size for datasets */
#define H5F_SIEVE_BUF_SIZE(F)   ((F)->shared->sieve_buf_size)
#else /* H5F_PACKAGE */
#define H5F_SIZEOF_ADDR(F)      (H5F_sizeof_addr(F))
#define H5F_SIZEOF_SIZE(F)      (H5F_sizeof_size(F))
#define H5F_SYM_LEAF_K(F)       (H5F_sym_leaf_k(F))
#define H5F_KVALUE(F,T)         (H5F_Kvalue(F,T))
#define H5F_RDCC_NELMTS(F)      (H5F_rdcc_nelmts(F))
#define H5F_RDCC_NBYTES(F)      (H5F_rdcc_nbytes(F))
#define H5F_RDCC_W0(F)          (H5F_rdcc_w0(F))
#define H5F_HAS_FEATURE(F,FL)   (H5F_has_feature(F,FL))
#define H5F_GRP_BTREE_SHARED(F) (H5F_grp_btree_shared(F))
#define H5F_BASE_ADDR(F)        (H5F_get_base_addr(F))
#define H5F_SIEVE_BUF_SIZE(F)   (H5F_sieve_buf_size(F))
#endif /* H5F_PACKAGE */


/* Macros to encode/decode offset/length's for storing in the file */
#ifdef NOT_YET
#define H5F_ENCODE_OFFSET(f,p,o) (H5F_SIZEOF_ADDR(f)==4 ? UINT32ENCODE(p,o) \
    : H5F_SIZEOF_ADDR(f)==8 ? UINT64ENCODE(p,o) \
    : H5F_SIZEOF_ADDR(f)==2 ? UINT16ENCODE(p,o) \
    : H5FPencode_unusual_offset(f,&(p),(uint8_t*)&(o)))
#else /* NOT_YET */
#define H5F_ENCODE_OFFSET(f,p,o) switch(H5F_SIZEOF_ADDR(f)) {          \
    case 4: UINT32ENCODE(p,o); break;                \
    case 8: UINT64ENCODE(p,o); break;                \
    case 2: UINT16ENCODE(p,o); break;                \
}
#endif /* NOT_YET */

#define H5F_DECODE_OFFSET(f,p,o) switch (H5F_SIZEOF_ADDR (f)) {  \
   case 4: UINT32DECODE (p, o);  break;              \
   case 8: UINT64DECODE (p, o);  break;              \
   case 2: UINT16DECODE (p, o);  break;              \
}

#define H5F_ENCODE_LENGTH(f,p,l) switch(H5F_SIZEOF_SIZE(f)) {   \
   case 4: UINT32ENCODE(p,l); break;                \
   case 8: UINT64ENCODE(p,l); break;                \
   case 2: UINT16ENCODE(p,l); break;                \
}

#define H5F_DECODE_LENGTH(f,p,l) switch(H5F_SIZEOF_SIZE(f)) {   \
   case 4: UINT32DECODE(p,l); break;                \
   case 8: UINT64DECODE(p,l); break;                \
   case 2: UINT16DECODE(p,l); break;                \
}

/*
 * Macros that check for overflows.  These are somewhat dangerous to fiddle
 * with.
 */
#if (H5_SIZEOF_SIZE_T >= H5_SIZEOF_OFF_T)
#   define H5F_OVERFLOW_SIZET2OFFT(X)                \
    ((size_t)(X)>=(size_t)((size_t)1<<(8*sizeof(off_t)-1)))
#else
#   define H5F_OVERFLOW_SIZET2OFFT(X) 0
#endif
#if (H5_SIZEOF_HSIZE_T >= H5_SIZEOF_OFF_T)
#   define H5F_OVERFLOW_HSIZET2OFFT(X)                \
    ((hsize_t)(X)>=(hsize_t)((hsize_t)1<<(8*sizeof(off_t)-1)))
#else
#   define H5F_OVERFLOW_HSIZET2OFFT(X) 0
#endif

/* ========= File Creation properties ============ */
/* Definitions for the size of the file user block in bytes */
#define H5F_CRT_USER_BLOCK_NAME      "block_size"
#define H5F_CRT_USER_BLOCK_SIZE      sizeof(hsize_t)
#define H5F_CRT_USER_BLOCK_DEF       0
/* Definitions for the 1/2 rank for symbol table leaf nodes */
#define H5F_CRT_SYM_LEAF_NAME        "symbol_leaf"
#define H5F_CRT_SYM_LEAF_SIZE        sizeof(unsigned)
#define H5F_CRT_SYM_LEAF_DEF         4
/* Definitions for the 1/2 rank for btree internal nodes    */
#define H5F_CRT_BTREE_RANK_NAME      "btree_rank"
#define H5F_CRT_BTREE_RANK_SIZE      sizeof(unsigned[H5B_NUM_BTREE_ID])
#define H5F_CRT_BTREE_RANK_DEF       {HDF5_BTREE_SNODE_IK_DEF,HDF5_BTREE_ISTORE_IK_DEF}
/* Definitions for byte number in an address                */
#define H5F_CRT_ADDR_BYTE_NUM_NAME   "addr_byte_num"
#define H5F_CRT_ADDR_BYTE_NUM_SIZE   sizeof(size_t)
#define H5F_CRT_ADDR_BYTE_NUM_DEF    sizeof(haddr_t)
/* Definitions for byte number for object size              */
#define H5F_CRT_OBJ_BYTE_NUM_NAME     "obj_byte_num"
#define H5F_CRT_OBJ_BYTE_NUM_SIZE     sizeof(size_t)
#define H5F_CRT_OBJ_BYTE_NUM_DEF      sizeof(hsize_t)
/* Definitions for version number of the superblock         */
#define H5F_CRT_SUPER_VERS_NAME       "super_version"
#define H5F_CRT_SUPER_VERS_SIZE       sizeof(unsigned)
#define H5F_CRT_SUPER_VERS_DEF        HDF5_SUPERBLOCK_VERSION_DEF
/* Definitions for free-space version number                */
#define H5F_CRT_FREESPACE_VERS_NAME   "free_space_version"
#define H5F_CRT_FREESPACE_VERS_SIZE   sizeof(unsigned)
#define H5F_CRT_FREESPACE_VERS_DEF    HDF5_FREESPACE_VERSION
/* Definitions for object directory version number          */
#define H5F_CRT_OBJ_DIR_VERS_NAME     "obj_dir_version"
#define H5F_CRT_OBJ_DIR_VERS_SIZE     sizeof(unsigned)
#define H5F_CRT_OBJ_DIR_VERS_DEF      HDF5_OBJECTDIR_VERSION
/* Definitions for shared-header format version             */
#define H5F_CRT_SHARE_HEAD_VERS_NAME  "share_head_version"
#define H5F_CRT_SHARE_HEAD_VERS_SIZE  sizeof(unsigned)
#define H5F_CRT_SHARE_HEAD_VERS_DEF   HDF5_SHAREDHEADER_VERSION

/* ========= File Access properties ============ */
/* Definitions for size of meta data cache(elements) */
#define H5F_ACS_META_CACHE_SIZE_NAME            "mdc_nelmts"
#define H5F_ACS_META_CACHE_SIZE_SIZE            sizeof(int)
#define H5F_ACS_META_CACHE_SIZE_DEF             H5AC_NSLOTS

/* Definitions for size of raw data chunk cache(elements) */
#define H5F_ACS_DATA_CACHE_ELMT_SIZE_NAME       "rdcc_nelmts"
#define H5F_ACS_DATA_CACHE_ELMT_SIZE_SIZE       sizeof(size_t)
#define H5F_ACS_DATA_CACHE_ELMT_SIZE_DEF        521

/* Definition for size of raw data chunk cache(bytes) */
#define H5F_ACS_DATA_CACHE_BYTE_SIZE_NAME       "rdcc_nbytes"
#define H5F_ACS_DATA_CACHE_BYTE_SIZE_SIZE       sizeof(size_t)
#define H5F_ACS_DATA_CACHE_BYTE_SIZE_DEF        (1024*1024)

/* Definition for preemption read chunks first */
#define H5F_ACS_PREEMPT_READ_CHUNKS_NAME        "rdcc_w0"
#define H5F_ACS_PREEMPT_READ_CHUNKS_SIZE        sizeof(double)
#define H5F_ACS_PREEMPT_READ_CHUNKS_DEF         0.75

/* Definition for threshold for alignment */
#define H5F_ACS_ALIGN_THRHD_NAME                "threshold"
#define H5F_ACS_ALIGN_THRHD_SIZE                sizeof(hsize_t)
#define H5F_ACS_ALIGN_THRHD_DEF                 1

/* Definition for alignment */
#define H5F_ACS_ALIGN_NAME                      "align"
#define H5F_ACS_ALIGN_SIZE                      sizeof(hsize_t)
#define H5F_ACS_ALIGN_DEF                       1

/* Definition for minimum metadata allocation block size (when
   aggregating metadata allocations. */
#define H5F_ACS_META_BLOCK_SIZE_NAME            "meta_block_size"
#define H5F_ACS_META_BLOCK_SIZE_SIZE            sizeof(hsize_t)
#define H5F_ACS_META_BLOCK_SIZE_DEF             2048

/* Definition for maximum sieve buffer size (when data sieving
   is allowed by file driver */
#define H5F_ACS_SIEVE_BUF_SIZE_NAME             "sieve_buf_size"
#define H5F_ACS_SIEVE_BUF_SIZE_SIZE             sizeof(size_t)
#define H5F_ACS_SIEVE_BUF_SIZE_DEF              (64*1024)

/* Definition for minimum "small data" allocation block size (when
   aggregating "small" raw data allocations. */
#define H5F_ACS_SDATA_BLOCK_SIZE_NAME           "sdata_block_size"
#define H5F_ACS_SDATA_BLOCK_SIZE_SIZE           sizeof(hsize_t)
#define H5F_ACS_SDATA_BLOCK_SIZE_DEF            2048

/* Definition for garbage-collect references */
#define H5F_ACS_GARBG_COLCT_REF_NAME            "gc_ref"
#define H5F_ACS_GARBG_COLCT_REF_SIZE            sizeof(unsigned)
#define H5F_ACS_GARBG_COLCT_REF_DEF             0

/* Definition for file driver ID */
#define H5F_ACS_FILE_DRV_ID_NAME                "driver_id"
#define H5F_ACS_FILE_DRV_ID_SIZE                sizeof(hid_t)
#define H5F_ACS_FILE_DRV_ID_DEF                 H5FD_SEC2

/* Definition for file driver info */
#define H5F_ACS_FILE_DRV_INFO_NAME              "driver_info"
#define H5F_ACS_FILE_DRV_INFO_SIZE              sizeof(void*)
#define H5F_ACS_FILE_DRV_INFO_DEF               NULL

/* Definition for file close degree */
#define H5F_CLOSE_DEGREE_NAME            "close_degree"
#define H5F_CLOSE_DEGREE_SIZE            sizeof(H5F_close_degree_t)
#define H5F_CLOSE_DEGREE_DEF            H5F_CLOSE_DEFAULT

/* Definition for offset position in file for family file driver */
#define H5F_ACS_FAMILY_OFFSET_NAME              "family_offset"
#define H5F_ACS_FAMILY_OFFSET_SIZE              sizeof(hsize_t)
#define H5F_ACS_FAMILY_OFFSET_DEF               0

/* Definition for data type in multi file driver */
#define H5F_ACS_MULTI_TYPE_NAME                 "multi_type"
#define H5F_ACS_MULTI_TYPE_SIZE                 sizeof(H5FD_mem_t)
#define H5F_ACS_MULTI_TYPE_DEF                  H5FD_MEM_DEFAULT

/* ======================== File Mount properties ====================*/
/* Definition for whether absolute symlinks local to file. */
#define H5F_MNT_SYM_LOCAL_NAME     "local"
#define H5F_MNT_SYM_LOCAL_SIZE    sizeof(hbool_t)
#define H5F_MNT_SYM_LOCAL_DEF     FALSE

#ifdef H5_HAVE_PARALLEL
/* Which process writes metadata */
#define H5_PAR_META_WRITE 0
#endif /* H5_HAVE_PARALLEL */

/* Forward declarations for prototype arguments */
struct H5B_class_t;
struct H5RC_t;

/* Private functions, not part of the publicly documented API */
H5_DLL herr_t H5F_init(void);
#ifdef NOT_YET
H5_DLL herr_t H5F_flush_all(hbool_t invalidate);
#endif /* NOT_YET */

/* Functions than retrieve values from the file struct */
H5_DLL hid_t H5F_get_driver_id(const H5F_t *f);
H5_DLL unsigned H5F_get_intent(const H5F_t *f);
H5_DLL herr_t H5F_get_fileno(const H5F_t *f, unsigned long *filenum);
H5_DLL hid_t H5F_get_id(H5F_t *file);
H5_DLL unsigned H5F_get_obj_count(const H5F_t *f, unsigned types);
H5_DLL unsigned H5F_get_obj_ids(const H5F_t *f, unsigned types, int max_objs, hid_t *obj_id_list);
H5_DLL haddr_t H5F_get_base_addr(const H5F_t *f);
H5_DLL haddr_t H5F_get_eoa(const H5F_t *f);
#ifdef H5_HAVE_PARALLEL
H5_DLL int H5F_mpi_get_rank(const H5F_t *f);
H5_DLL MPI_Comm H5F_mpi_get_comm(const H5F_t *f);
#endif /* H5_HAVE_PARALLEL */

/* Functions than check file mounting information */
H5_DLL htri_t H5F_is_mount(const H5F_t *file);
H5_DLL htri_t H5F_has_mount(const H5F_t *file);

/* Functions than retrieve values set from the FCPL */
H5_DLL size_t H5F_sizeof_addr(const H5F_t *f);
H5_DLL size_t H5F_sizeof_size(const H5F_t *f);
H5_DLL unsigned H5F_sym_leaf_k(const H5F_t *f);
H5_DLL unsigned H5F_Kvalue(const H5F_t *f, const struct H5B_class_t *type);
H5_DLL hbool_t H5F_has_feature(const H5F_t *f, unsigned feature);
H5_DLL size_t H5F_rdcc_nbytes(const H5F_t *f);
H5_DLL size_t H5F_rdcc_nelmts(const H5F_t *f);
H5_DLL double H5F_rdcc_w0(const H5F_t *f);
H5_DLL struct H5RC_t *H5F_grp_btree_shared(const H5F_t *f);
H5_DLL size_t H5F_sieve_buf_size(const H5F_t *f);

/* Functions that operate on blocks of bytes wrt super block */
H5_DLL herr_t H5F_block_read(const H5F_t *f, H5FD_mem_t type, haddr_t addr,
                size_t size, hid_t dxpl_id, void *buf/*out*/);
H5_DLL herr_t H5F_block_write(const H5F_t *f, H5FD_mem_t type, haddr_t addr,
                size_t size, hid_t dxpl_id, const void *buf);

/* Address-related functions */
H5_DLL void H5F_addr_encode(const H5F_t *, uint8_t** /*in,out*/, haddr_t);
H5_DLL void H5F_addr_decode(const H5F_t *, const uint8_t** /*in,out*/,
           haddr_t* /*out*/);
H5_DLL herr_t H5F_addr_pack(H5F_t *f, haddr_t *addr_p /*out*/,
           const unsigned long objno[2]);

/* Callback functions for file access class */
H5_DLL herr_t H5F_acs_create(hid_t fapl_id, void *close_data);
H5_DLL herr_t H5F_acs_close(hid_t fapl_id, void *close_data);
H5_DLL herr_t H5F_acs_copy(hid_t new_fapl_id, hid_t old_fapl_id,
                            void *close_data);

/* Shared file list related routines */
H5_DLL herr_t H5F_sfile_assert_num(unsigned n);

/* Debugging functions */
H5_DLL herr_t H5F_debug(H5F_t *f, hid_t dxpl_id, FILE * stream, int indent, int fwidth);

#endif
