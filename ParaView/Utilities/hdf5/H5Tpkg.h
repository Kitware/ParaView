/*
 * Copyright (C) 1997 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, December  8, 1997
 *
 * Purpose:     This file contains declarations which are visible only within
 *              the H5T package.  Source files outside the H5T package should
 *              include H5Tprivate.h instead.
 */
#ifndef H5T_PACKAGE
#error "Do not include this file outside the H5T package!"
#endif

#ifndef _H5Tpkg_H
#define _H5Tpkg_H

/*
 * Define this to enable debugging.
 */
#ifdef NDEBUG
#  undef H5T_DEBUG
#endif

#include "H5HGprivate.h"
#include "H5Dprivate.h"
#include "H5Fprivate.h"
#include "H5Rprivate.h"
#include "H5Tprivate.h"

typedef struct H5T_atomic_t {
    H5T_order_t         order;  /*byte order                                 */
    size_t              prec;   /*precision in bits                          */
    size_t              offset; /*bit position of lsb of value               */
    H5T_pad_t           lsb_pad;/*type of lsb padding                        */
    H5T_pad_t           msb_pad;/*type of msb padding                        */
    union {
        struct {
            H5T_sign_t  sign;   /*type of integer sign                       */
        } i;                    /*integer; integer types                     */

        struct {
            size_t      sign;   /*bit position of sign bit                   */
            size_t      epos;   /*position of lsb of exponent                */
            size_t      esize;  /*size of exponent in bits                   */
            uint64_t    ebias;  /*exponent bias                              */
            size_t      mpos;   /*position of lsb of mantissa                */
            size_t      msize;  /*size of mantissa                           */
            H5T_norm_t  norm;   /*normalization                              */
            H5T_pad_t   pad;    /*type of padding for internal bits          */
        } f;                    /*floating-point types                       */

        struct {
            H5T_cset_t  cset;   /*character set                              */
            H5T_str_t   pad;    /*space or null padding of extra bytes       */
        } s;                    /*string types                               */

        struct {
            H5R_type_t  rtype;  /*type of reference stored                   */
        } r;                    /*reference types                            */
    } u;
} H5T_atomic_t;

/* How members are sorted for compound or enum data types */
typedef enum H5T_sort_t {
    H5T_SORT_NONE       = 0,            /*not sorted                         */
    H5T_SORT_NAME       = 1,            /*sorted by member name              */
    H5T_SORT_VALUE      = 2             /*sorted by memb offset or enum value*/
} H5T_sort_t;

/* A compound data type */
typedef struct H5T_compnd_t {
    int         nalloc;         /*num entries allocated in MEMB array*/
    int         nmembs;         /*number of members defined in struct*/
    H5T_sort_t          sorted;         /*how are members sorted?            */
    hbool_t     has_array;  /* Set if this type has an array datatype member */
                            /* and should be written with the new version of */
                            /* the datatype object header message */
    struct H5T_cmemb_t  *memb;          /*array of struct members            */
} H5T_compnd_t;

/* An enumeration data type */
typedef struct H5T_enum_t {
    int         nalloc;         /*num entries allocated              */
    int         nmembs;         /*number of members defined in enum  */
    H5T_sort_t          sorted;         /*how are members sorted?            */
    uint8_t             *value;         /*array of values                    */
    char                **name;         /*array of symbol names              */
} H5T_enum_t;

/* VL function pointers */
typedef hssize_t (*H5T_vlen_getlenfunc_t)(H5F_t *f, void *vl_addr);
typedef herr_t (*H5T_vlen_readfunc_t)(H5F_t *f, void *vl_addr, void *buf, size_t len);
typedef herr_t (*H5T_vlen_writefunc_t)(const H5D_xfer_t *xfer_parms, H5F_t *f, void *vl_addr, void *buf, hsize_t seq_len, hsize_t base_size);

/* A VL datatype */
typedef struct H5T_vlen_t {
    H5T_vlen_type_t     type;   /* Type of VL data in buffer */
    H5T_vlen_loc_t      loc;    /* Location of VL data in buffer */
    H5F_t *f;                   /* File ID (if VL data is on disk) */
    H5T_vlen_getlenfunc_t getlen;   /* Function to get VL sequence size (in element units, not bytes) */
    H5T_vlen_readfunc_t read;   /* Function to read VL sequence into buffer */
    H5T_vlen_writefunc_t write; /* Function to write VL sequence from buffer */
} H5T_vlen_t;

/* An opaque data type */
typedef struct H5T_opaque_t {
    char                *tag;           /*short type description string      */
} H5T_opaque_t;

/* An array datatype */
typedef struct H5T_array_t {
    size_t              nelem;          /* total number of elements in array */
    int         ndims;          /* member dimensionality        */
    size_t              dim[H5S_MAX_RANK];  /* size in each dimension       */
    int         perm[H5S_MAX_RANK]; /* index permutation            */
} H5T_array_t;

typedef enum H5T_state_t {
    H5T_STATE_TRANSIENT,                /*type is a modifiable transient     */
    H5T_STATE_RDONLY,                   /*transient, not modifiable, closable*/
    H5T_STATE_IMMUTABLE,                /*constant, not closable             */
    H5T_STATE_NAMED,                    /*named constant, not open           */
    H5T_STATE_OPEN                      /*named constant, open object header */
} H5T_state_t;

struct H5T_t {
    H5T_state_t         state;  /*current state of the type                  */
    H5G_entry_t         ent;    /*the type is a named type                   */
    H5F_t               *sh_file;/*file pointer if this is a shared type     */
    H5T_class_t         type;   /*which class of type is this?               */
    size_t              size;   /*total size of an instance of this type     */
    hbool_t     force_conv; /* Set if this type always needs to be converted and H5T_conv_noop cannot be called */
    struct H5T_t        *parent;/*parent type for derived data types         */
    union {
        H5T_atomic_t    atomic; /* an atomic data type              */
        H5T_compnd_t    compnd; /* a compound data type (struct)    */
        H5T_enum_t          enumer; /* an enumeration type (enum)       */
        H5T_vlen_t          vlen;   /* a variable-length datatype       */
        H5T_opaque_t    opaque; /* an opaque data type              */
        H5T_array_t         array;  /* an array datatype                */
    } u;
};

/* A compound data type member */
typedef struct H5T_cmemb_t {
    char                *name;          /*name of this member                */
    size_t              offset;         /*offset from beginning of struct    */
    size_t              size;           /*total size: dims * type_size       */
    struct H5T_t        *type;          /*type of this member                */
} H5T_cmemb_t;

/* The master list of soft conversion functions */
typedef struct H5T_soft_t {
    char        name[H5T_NAMELEN];      /*name for debugging only            */
    H5T_class_t src;                    /*source data type class             */
    H5T_class_t dst;                    /*destination data type class        */
    H5T_conv_t  func;                   /*the conversion function            */
} H5T_soft_t;

/* Bit search direction */
typedef enum H5T_sdir_t {
    H5T_BIT_LSB,                        /*search lsb toward msb              */
    H5T_BIT_MSB                         /*search msb toward lsb              */
} H5T_sdir_t;

/* The overflow handler */
__DLLVAR__ H5T_overflow_t H5T_overflow_g;

/*
 * Alignment information for native types. A value of N indicates that the
 * data must be aligned on an address ADDR such that 0 == ADDR mod N. When
 * N=1 no alignment is required; N=0 implies that alignment constraints were
 * not calculated.
 */
__DLLVAR__ size_t       H5T_NATIVE_SCHAR_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UCHAR_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_SHORT_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_USHORT_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_INT_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_LONG_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_ULONG_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_LLONG_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_ULLONG_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_FLOAT_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_DOUBLE_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_LDOUBLE_ALIGN_g;

/* C9x alignment constraints */
__DLLVAR__ size_t       H5T_NATIVE_INT8_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT8_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_INT_LEAST8_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT_LEAST8_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_INT_FAST8_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT_FAST8_ALIGN_g;

__DLLVAR__ size_t       H5T_NATIVE_INT16_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT16_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_INT_LEAST16_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT_LEAST16_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_INT_FAST16_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT_FAST16_ALIGN_g;

__DLLVAR__ size_t       H5T_NATIVE_INT32_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT32_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_INT_LEAST32_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT_LEAST32_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_INT_FAST32_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT_FAST32_ALIGN_g;

__DLLVAR__ size_t       H5T_NATIVE_INT64_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT64_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_INT_LEAST64_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT_LEAST64_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_INT_FAST64_ALIGN_g;
__DLLVAR__ size_t       H5T_NATIVE_UINT_FAST64_ALIGN_g;

/* Conversion functions */
__DLL__ herr_t H5T_conv_noop(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                             hsize_t nelmts, size_t buf_stride,
                             size_t bkg_stride, void *buf, void *bkg,
                             hid_t dset_xfer_plist);

__DLL__ herr_t H5T_conv_order(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                              hsize_t nelmts, size_t buf_stride,
                              size_t bkg_stride, void *_buf, void *bkg,
                              hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_order_opt(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *_buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_struct(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                               hsize_t nelmts, size_t buf_stride,
                               size_t bkg_stride, void *_buf, void *bkg,
                               hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_struct_opt(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *_buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_enum(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                             hsize_t nelmts, size_t buf_stride,
                             size_t bkg_stride, void *buf, void *bkg,
                             hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_vlen(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                             hsize_t nelmts, size_t buf_stride,
                             size_t bkg_stride, void *buf, void *bkg,
                             hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_array(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                             hsize_t nelmts, size_t buf_stride,
                             size_t bkg_stride, void *buf, void *bkg,
                             hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_i_i(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            hsize_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg,
                            hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_f_f(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            hsize_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg,
                            hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_s_s(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            hsize_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg,
                            hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_b_b(hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
                            hsize_t nelmts, size_t buf_stride,
                            size_t bkg_stride, void *_buf, void *bkg,
                            hid_t dset_xfer_plist);

__DLL__ herr_t H5T_conv_schar_uchar(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uchar_schar(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_schar_short(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_schar_ushort(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uchar_short(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uchar_ushort(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_schar_int(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_schar_uint(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uchar_int(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uchar_uint(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_schar_long(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_schar_ulong(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uchar_long(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uchar_ulong(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_schar_llong(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_schar_ullong(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uchar_llong(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uchar_ullong(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);

__DLL__ herr_t H5T_conv_short_schar(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_short_uchar(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ushort_schar(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ushort_uchar(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_short_ushort(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ushort_short(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_short_int(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_short_uint(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ushort_int(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ushort_uint(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_short_long(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_short_ulong(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ushort_long(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ushort_ulong(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_short_llong(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_short_ullong(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ushort_llong(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ushort_ullong(hid_t src_id, hid_t dst_id,
                                      H5T_cdata_t *cdata, hsize_t nelmts,
                                      size_t buf_stride, size_t bkg_stride,
                                      void *buf, void *bkg,
                                      hid_t dset_xfer_plist);

__DLL__ herr_t H5T_conv_int_schar(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_int_uchar(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uint_schar(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uint_uchar(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_int_short(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_int_ushort(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uint_short(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uint_ushort(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_int_uint(hid_t src_id, hid_t dst_id,
                                 H5T_cdata_t *cdata, hsize_t nelmts,
                                 size_t buf_stride, size_t bkg_stride,
                                 void *buf, void *bkg,
                                 hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uint_int(hid_t src_id, hid_t dst_id,
                                 H5T_cdata_t *cdata, hsize_t nelmts,
                                 size_t buf_stride, size_t bkg_stride,
                                 void *buf, void *bkg,
                                 hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_int_long(hid_t src_id, hid_t dst_id,
                                 H5T_cdata_t *cdata, hsize_t nelmts,
                                 size_t buf_stride, size_t bkg_stride,
                                 void *buf, void *bkg,
                                 hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_int_ulong(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uint_long(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uint_ulong(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_int_llong(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_int_ullong(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uint_llong(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_uint_ullong(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);

__DLL__ herr_t H5T_conv_long_schar(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_long_uchar(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ulong_schar(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ulong_uchar(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_long_short(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_long_ushort(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ulong_short(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ulong_ushort(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_long_int(hid_t src_id, hid_t dst_id,
                                 H5T_cdata_t *cdata, hsize_t nelmts,
                                 size_t buf_stride, size_t bkg_stride,
                                 void *buf, void *bkg,
                                 hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_long_uint(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ulong_int(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ulong_uint(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_long_ulong(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ulong_long(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_long_llong(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_long_ullong(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ulong_llong(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ulong_ullong(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);

__DLL__ herr_t H5T_conv_llong_schar(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_llong_uchar(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ullong_schar(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ullong_uchar(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_llong_short(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_llong_ushort(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ullong_short(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ullong_ushort(hid_t src_id, hid_t dst_id,
                                      H5T_cdata_t *cdata, hsize_t nelmts,
                                      size_t buf_stride, size_t bkg_stride,
                                      void *buf, void *bkg,
                                      hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_llong_int(hid_t src_id, hid_t dst_id,
                                  H5T_cdata_t *cdata, hsize_t nelmts,
                                  size_t buf_stride, size_t bkg_stride,
                                  void *buf, void *bkg,
                                  hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_llong_uint(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ullong_int(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ullong_uint(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_llong_long(hid_t src_id, hid_t dst_id,
                                   H5T_cdata_t *cdata, hsize_t nelmts,
                                   size_t buf_stride, size_t bkg_stride,
                                   void *buf, void *bkg,
                                   hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_llong_ulong(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ullong_long(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *buf, void *bkg,
                                    hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ullong_ulong(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_llong_ullong(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_ullong_llong(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);

__DLL__ herr_t H5T_conv_float_double(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_double_float(hid_t src_id, hid_t dst_id,
                                     H5T_cdata_t *cdata, hsize_t nelmts,
                                     size_t buf_stride, size_t bkg_stride,
                                     void *buf, void *bkg,
                                     hid_t dset_xfer_plist);
__DLL__ herr_t H5T_conv_i32le_f64le(hid_t src_id, hid_t dst_id,
                                    H5T_cdata_t *cdata, hsize_t nelmts,
                                    size_t buf_stride, size_t bkg_stride,
                                    void *_buf, void *bkg,
                                    hid_t dset_xfer_plist);

/* Bit twiddling functions */
__DLL__ void H5T_bit_copy(uint8_t *dst, size_t dst_offset, const uint8_t *src,
                          size_t src_offset, size_t size);
__DLL__ void H5T_bit_set(uint8_t *buf, size_t offset, size_t size,
                         hbool_t value);
__DLL__ hsize_t H5T_bit_get_d(uint8_t *buf, size_t offset, size_t size);
__DLL__ void H5T_bit_set_d(uint8_t *buf, size_t offset, size_t size,
                           hsize_t val);
__DLL__ ssize_t H5T_bit_find(uint8_t *buf, size_t offset, size_t size,
                             H5T_sdir_t direction, hbool_t value);
__DLL__ htri_t H5T_bit_inc(uint8_t *buf, size_t start, size_t size);

/* VL functions */
__DLL__ hssize_t H5T_vlen_seq_mem_getlen(H5F_t *f, void *vl_addr);
__DLL__ herr_t H5T_vlen_seq_mem_read(H5F_t *f, void *vl_addr, void *_buf, size_t len);
__DLL__ herr_t H5T_vlen_seq_mem_write(const H5D_xfer_t *xfer_parms, H5F_t *f, void *vl_addr, void *_buf, hsize_t seq_len, hsize_t base_size);
__DLL__ hssize_t H5T_vlen_str_mem_getlen(H5F_t *f, void *vl_addr);
__DLL__ herr_t H5T_vlen_str_mem_read(H5F_t *f, void *vl_addr, void *_buf, size_t len);
__DLL__ herr_t H5T_vlen_str_mem_write(const H5D_xfer_t *xfer_parms, H5F_t *f, void *vl_addr, void *_buf, hsize_t seq_len, hsize_t base_size);
__DLL__ hssize_t H5T_vlen_disk_getlen(H5F_t *f, void *vl_addr);
__DLL__ herr_t H5T_vlen_disk_read(H5F_t *f, void *vl_addr, void *_buf, size_t len);
__DLL__ herr_t H5T_vlen_disk_write(const H5D_xfer_t *xfer_parms, H5F_t *f, void *vl_addr, void *_buf, hsize_t seq_len, hsize_t base_size);

/* Array functions */
__DLL__ H5T_t * H5T_array_create(H5T_t *base, int ndims,
        const hsize_t dim[/* ndims */], const int perm[/* ndims */]);

#endif
