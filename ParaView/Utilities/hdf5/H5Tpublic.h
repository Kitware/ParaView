/****************************************************************************
 * NCSA HDF                                                                 *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 *                                                                          *
 * For conditions of distribution and use, see the accompanying             *
 * hdf/COPYING file.                                                        *
 *                                                                          *
 ****************************************************************************/

/*
 * This file contains public declarations for the H5T module.
 */
#ifndef _H5Tpublic_H
#define _H5Tpublic_H

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Ipublic.h"

#define HOFFSET(S,M)    (offsetof(S,M))

/* These are the various classes of data types */
/* If this goes over 16 types (0-15), the file format will need to change) */
typedef enum H5T_class_t {
    H5T_NO_CLASS         = -1,  /*error                                      */
    H5T_INTEGER          = 0,   /*integer types                              */
    H5T_FLOAT            = 1,   /*floating-point types                       */
    H5T_TIME             = 2,   /*date and time types                        */
    H5T_STRING           = 3,   /*character string types                     */
    H5T_BITFIELD         = 4,   /*bit field types                            */
    H5T_OPAQUE           = 5,   /*opaque types                               */
    H5T_COMPOUND         = 6,   /*compound types                             */
    H5T_REFERENCE        = 7,   /*reference types                            */
    H5T_ENUM             = 8,   /*enumeration types                          */
    H5T_VLEN             = 9,   /*Variable-Length types                      */
    H5T_ARRAY            = 10,  /*Array types                                */

    H5T_NCLASSES                /*this must be last                          */
} H5T_class_t;

/* Byte orders */
typedef enum H5T_order_t {
    H5T_ORDER_ERROR      = -1,  /*error                                      */
    H5T_ORDER_LE         = 0,   /*little endian                              */
    H5T_ORDER_BE         = 1,   /*bit endian                                 */
    H5T_ORDER_VAX        = 2,   /*VAX mixed endian                           */
    H5T_ORDER_NONE       = 3    /*no particular order (strings, bits,..)     */
    /*H5T_ORDER_NONE must be last */
} H5T_order_t;

/* Types of integer sign schemes */
typedef enum H5T_sign_t {
    H5T_SGN_ERROR        = -1,  /*error                                      */
    H5T_SGN_NONE         = 0,   /*this is an unsigned type                   */
    H5T_SGN_2            = 1,   /*two's complement                           */

    H5T_NSGN             = 2    /*this must be last!                         */
} H5T_sign_t;

/* Floating-point normalization schemes */
typedef enum H5T_norm_t {
    H5T_NORM_ERROR       = -1,  /*error                                      */
    H5T_NORM_IMPLIED     = 0,   /*msb of mantissa isn't stored, always 1     */
    H5T_NORM_MSBSET      = 1,   /*msb of mantissa is always 1                */
    H5T_NORM_NONE        = 2    /*not normalized                             */
    /*H5T_NORM_NONE must be last */
} H5T_norm_t;

/*
 * Character set to use for text strings.  Do not change these values since
 * they appear in HDF5 files!
 */
typedef enum H5T_cset_t {
    H5T_CSET_ERROR       = -1,  /*error                                      */
    H5T_CSET_ASCII       = 0,   /*US ASCII                                   */
    H5T_CSET_RESERVED_1  = 1,   /*reserved for later use                     */
    H5T_CSET_RESERVED_2  = 2,   /*reserved for later use                     */
    H5T_CSET_RESERVED_3  = 3,   /*reserved for later use                     */
    H5T_CSET_RESERVED_4  = 4,   /*reserved for later use                     */
    H5T_CSET_RESERVED_5  = 5,   /*reserved for later use                     */
    H5T_CSET_RESERVED_6  = 6,   /*reserved for later use                     */
    H5T_CSET_RESERVED_7  = 7,   /*reserved for later use                     */
    H5T_CSET_RESERVED_8  = 8,   /*reserved for later use                     */
    H5T_CSET_RESERVED_9  = 9,   /*reserved for later use                     */
    H5T_CSET_RESERVED_10 = 10,  /*reserved for later use                     */
    H5T_CSET_RESERVED_11 = 11,  /*reserved for later use                     */
    H5T_CSET_RESERVED_12 = 12,  /*reserved for later use                     */
    H5T_CSET_RESERVED_13 = 13,  /*reserved for later use                     */
    H5T_CSET_RESERVED_14 = 14,  /*reserved for later use                     */
    H5T_CSET_RESERVED_15 = 15   /*reserved for later use                     */
} H5T_cset_t;
#define H5T_NCSET 1             /*Number of character sets actually defined  */

/*
 * Type of padding to use in character strings.  Do not change these values
 * since they appear in HDF5 files!
 */
typedef enum H5T_str_t {
    H5T_STR_ERROR        = -1,  /*error                                      */
    H5T_STR_NULLTERM     = 0,   /*null terminate like in C                   */
    H5T_STR_NULLPAD      = 1,   /*pad with nulls                             */
    H5T_STR_SPACEPAD     = 2,   /*pad with spaces like in Fortran            */
    H5T_STR_RESERVED_3   = 3,   /*reserved for later use                     */
    H5T_STR_RESERVED_4   = 4,   /*reserved for later use                     */
    H5T_STR_RESERVED_5   = 5,   /*reserved for later use                     */
    H5T_STR_RESERVED_6   = 6,   /*reserved for later use                     */
    H5T_STR_RESERVED_7   = 7,   /*reserved for later use                     */
    H5T_STR_RESERVED_8   = 8,   /*reserved for later use                     */
    H5T_STR_RESERVED_9   = 9,   /*reserved for later use                     */
    H5T_STR_RESERVED_10  = 10,  /*reserved for later use                     */
    H5T_STR_RESERVED_11  = 11,  /*reserved for later use                     */
    H5T_STR_RESERVED_12  = 12,  /*reserved for later use                     */
    H5T_STR_RESERVED_13  = 13,  /*reserved for later use                     */
    H5T_STR_RESERVED_14  = 14,  /*reserved for later use                     */
    H5T_STR_RESERVED_15  = 15   /*reserved for later use                     */
} H5T_str_t;
#define H5T_NSTR 3              /*num H5T_str_t types actually defined       */

/* Type of padding to use in other atomic types */
typedef enum H5T_pad_t {
    H5T_PAD_ERROR        = -1,  /*error                                      */
    H5T_PAD_ZERO         = 0,   /*always set to zero                         */
    H5T_PAD_ONE          = 1,   /*always set to one                          */
    H5T_PAD_BACKGROUND   = 2,   /*set to background value                    */

    H5T_NPAD             = 3    /*THIS MUST BE LAST                          */
} H5T_pad_t;

/* Commands sent to conversion functions */
typedef enum H5T_cmd_t {
    H5T_CONV_INIT       = 0,    /*query and/or initialize private data       */
    H5T_CONV_CONV       = 1,    /*convert data from source to dest data type */
    H5T_CONV_FREE       = 2     /*function is being removed from path        */
} H5T_cmd_t;

/* Type conversion client data */
typedef struct H5T_cdata_t {
    H5T_cmd_t           command;/*what should the conversion function do?    */
    H5T_bkg_t           need_bkg;/*is the background buffer needed?          */
    hbool_t             recalc; /*recalculate private data                   */
    void                *priv;  /*private data                               */
} H5T_cdata_t;

/* Conversion function persistence */
typedef enum H5T_pers_t {
    H5T_PERS_DONTCARE   = -1,   /*wild card                                  */
    H5T_PERS_HARD       = 0,    /*hard conversion function                   */
    H5T_PERS_SOFT       = 1     /*soft conversion function                   */
} H5T_pers_t;

/* Variable Length Datatype struct in memory */
/* (This is only used for VL sequences, not VL strings, which are stored in char *'s) */
typedef struct {
    size_t len; /* Length of VL data (in base type units) */
    void *p;    /* Pointer to VL data */
} hvl_t;

/* Variable Length String information */
#define H5T_VARIABLE    ((size_t)(-1))  /* Indicate that a string is variable length (null-terminated in C, instead of fixed length) */

/* All data type conversion functions are... */
typedef herr_t (*H5T_conv_t) (hid_t src_id, hid_t dst_id, H5T_cdata_t *cdata,
      hsize_t nelmts, size_t buf_stride, size_t bkg_stride, void *buf,
      void *bkg, hid_t dset_xfer_plist);

/*
 * If an error occurs during a data type conversion then the function
 * registered with H5Tset_overflow() is called.  It's arguments are the
 * source and destination data types, a buffer which has the source value,
 * and a buffer to receive an optional result for the overflow conversion.
 * If the overflow handler chooses a value for the result it should return
 * non-negative; otherwise the hdf5 library will choose an appropriate
 * result.
 */
typedef herr_t (*H5T_overflow_t)(hid_t src_id, hid_t dst_id,
                                 void *src_buf, void *dst_buf);


#ifdef __cplusplus
extern "C" {
#endif

/*
 * The IEEE floating point types in various byte orders.
 */
#define H5T_IEEE_F32BE          (H5open(), H5T_IEEE_F32BE_g)
#define H5T_IEEE_F32LE          (H5open(), H5T_IEEE_F32LE_g)
#define H5T_IEEE_F64BE          (H5open(), H5T_IEEE_F64BE_g)
#define H5T_IEEE_F64LE          (H5open(), H5T_IEEE_F64LE_g)
__DLLVAR__ hid_t H5T_IEEE_F32BE_g;
__DLLVAR__ hid_t H5T_IEEE_F32LE_g;
__DLLVAR__ hid_t H5T_IEEE_F64BE_g;
__DLLVAR__ hid_t H5T_IEEE_F64LE_g;

/*
 * These are "standard" types.  For instance, signed (2's complement) and
 * unsigned integers of various sizes and byte orders.
 */
#define H5T_STD_I8BE            (H5open(), H5T_STD_I8BE_g)
#define H5T_STD_I8LE            (H5open(), H5T_STD_I8LE_g)
#define H5T_STD_I16BE           (H5open(), H5T_STD_I16BE_g)
#define H5T_STD_I16LE           (H5open(), H5T_STD_I16LE_g)
#define H5T_STD_I32BE           (H5open(), H5T_STD_I32BE_g)
#define H5T_STD_I32LE           (H5open(), H5T_STD_I32LE_g)
#define H5T_STD_I64BE           (H5open(), H5T_STD_I64BE_g)
#define H5T_STD_I64LE           (H5open(), H5T_STD_I64LE_g)
#define H5T_STD_U8BE            (H5open(), H5T_STD_U8BE_g)
#define H5T_STD_U8LE            (H5open(), H5T_STD_U8LE_g)
#define H5T_STD_U16BE           (H5open(), H5T_STD_U16BE_g)
#define H5T_STD_U16LE           (H5open(), H5T_STD_U16LE_g)
#define H5T_STD_U32BE           (H5open(), H5T_STD_U32BE_g)
#define H5T_STD_U32LE           (H5open(), H5T_STD_U32LE_g)
#define H5T_STD_U64BE           (H5open(), H5T_STD_U64BE_g)
#define H5T_STD_U64LE           (H5open(), H5T_STD_U64LE_g)
#define H5T_STD_B8BE            (H5open(), H5T_STD_B8BE_g)
#define H5T_STD_B8LE            (H5open(), H5T_STD_B8LE_g)
#define H5T_STD_B16BE           (H5open(), H5T_STD_B16BE_g)
#define H5T_STD_B16LE           (H5open(), H5T_STD_B16LE_g)
#define H5T_STD_B32BE           (H5open(), H5T_STD_B32BE_g)
#define H5T_STD_B32LE           (H5open(), H5T_STD_B32LE_g)
#define H5T_STD_B64BE           (H5open(), H5T_STD_B64BE_g)
#define H5T_STD_B64LE           (H5open(), H5T_STD_B64LE_g)
#define H5T_STD_REF_OBJ     (H5open(), H5T_STD_REF_OBJ_g)
#define H5T_STD_REF_DSETREG (H5open(), H5T_STD_REF_DSETREG_g)
__DLLVAR__ hid_t H5T_STD_I8BE_g;
__DLLVAR__ hid_t H5T_STD_I8LE_g;
__DLLVAR__ hid_t H5T_STD_I16BE_g;
__DLLVAR__ hid_t H5T_STD_I16LE_g;
__DLLVAR__ hid_t H5T_STD_I32BE_g;
__DLLVAR__ hid_t H5T_STD_I32LE_g;
__DLLVAR__ hid_t H5T_STD_I64BE_g;
__DLLVAR__ hid_t H5T_STD_I64LE_g;
__DLLVAR__ hid_t H5T_STD_U8BE_g;
__DLLVAR__ hid_t H5T_STD_U8LE_g;
__DLLVAR__ hid_t H5T_STD_U16BE_g;
__DLLVAR__ hid_t H5T_STD_U16LE_g;
__DLLVAR__ hid_t H5T_STD_U32BE_g;
__DLLVAR__ hid_t H5T_STD_U32LE_g;
__DLLVAR__ hid_t H5T_STD_U64BE_g;
__DLLVAR__ hid_t H5T_STD_U64LE_g;
__DLLVAR__ hid_t H5T_STD_B8BE_g;
__DLLVAR__ hid_t H5T_STD_B8LE_g;
__DLLVAR__ hid_t H5T_STD_B16BE_g;
__DLLVAR__ hid_t H5T_STD_B16LE_g;
__DLLVAR__ hid_t H5T_STD_B32BE_g;
__DLLVAR__ hid_t H5T_STD_B32LE_g;
__DLLVAR__ hid_t H5T_STD_B64BE_g;
__DLLVAR__ hid_t H5T_STD_B64LE_g;
__DLLVAR__ hid_t H5T_STD_REF_OBJ_g;
__DLLVAR__ hid_t H5T_STD_REF_DSETREG_g;

/*
 * Types which are particular to Unix.
 */
#define H5T_UNIX_D32BE          (H5open(), H5T_UNIX_D32BE_g)
#define H5T_UNIX_D32LE          (H5open(), H5T_UNIX_D32LE_g)
#define H5T_UNIX_D64BE          (H5open(), H5T_UNIX_D64BE_g)
#define H5T_UNIX_D64LE          (H5open(), H5T_UNIX_D64LE_g)
__DLLVAR__ hid_t H5T_UNIX_D32BE_g;
__DLLVAR__ hid_t H5T_UNIX_D32LE_g;
__DLLVAR__ hid_t H5T_UNIX_D64BE_g;
__DLLVAR__ hid_t H5T_UNIX_D64LE_g;

/*
 * Types particular to the C language.  String types use `bytes' instead
 * of `bits' as their size.
 */
#define H5T_C_S1                (H5open(), H5T_C_S1_g)
__DLLVAR__ hid_t H5T_C_S1_g;

/*
 * Types particular to Fortran.
 */
#define H5T_FORTRAN_S1          (H5open(), H5T_FORTRAN_S1_g)
__DLLVAR__ hid_t H5T_FORTRAN_S1_g;

/*
 * These types are for Intel CPU's.  They are little endian with IEEE
 * floating point.
 */
#define H5T_INTEL_I8            H5T_STD_I8LE
#define H5T_INTEL_I16           H5T_STD_I16LE
#define H5T_INTEL_I32           H5T_STD_I32LE
#define H5T_INTEL_I64           H5T_STD_I64LE
#define H5T_INTEL_U8            H5T_STD_U8LE
#define H5T_INTEL_U16           H5T_STD_U16LE
#define H5T_INTEL_U32           H5T_STD_U32LE
#define H5T_INTEL_U64           H5T_STD_U64LE
#define H5T_INTEL_B8            H5T_STD_B8LE
#define H5T_INTEL_B16           H5T_STD_B16LE
#define H5T_INTEL_B32           H5T_STD_B32LE
#define H5T_INTEL_B64           H5T_STD_B64LE
#define H5T_INTEL_F32           H5T_IEEE_F32LE
#define H5T_INTEL_F64           H5T_IEEE_F64LE

/*
 * These types are for DEC Alpha CPU's.  They are little endian with IEEE
 * floating point.
 */
#define H5T_ALPHA_I8            H5T_STD_I8LE
#define H5T_ALPHA_I16           H5T_STD_I16LE
#define H5T_ALPHA_I32           H5T_STD_I32LE
#define H5T_ALPHA_I64           H5T_STD_I64LE
#define H5T_ALPHA_U8            H5T_STD_U8LE
#define H5T_ALPHA_U16           H5T_STD_U16LE
#define H5T_ALPHA_U32           H5T_STD_U32LE
#define H5T_ALPHA_U64           H5T_STD_U64LE
#define H5T_ALPHA_B8            H5T_STD_B8LE
#define H5T_ALPHA_B16           H5T_STD_B16LE
#define H5T_ALPHA_B32           H5T_STD_B32LE
#define H5T_ALPHA_B64           H5T_STD_B64LE
#define H5T_ALPHA_F32           H5T_IEEE_F32LE
#define H5T_ALPHA_F64           H5T_IEEE_F64LE

/*
 * These types are for MIPS cpu's commonly used in SGI systems. They are big
 * endian with IEEE floating point.
 */
#define H5T_MIPS_I8             H5T_STD_I8BE
#define H5T_MIPS_I16            H5T_STD_I16BE
#define H5T_MIPS_I32            H5T_STD_I32BE
#define H5T_MIPS_I64            H5T_STD_I64BE
#define H5T_MIPS_U8             H5T_STD_U8BE
#define H5T_MIPS_U16            H5T_STD_U16BE
#define H5T_MIPS_U32            H5T_STD_U32BE
#define H5T_MIPS_U64            H5T_STD_U64BE
#define H5T_MIPS_B8             H5T_STD_B8BE
#define H5T_MIPS_B16            H5T_STD_B16BE
#define H5T_MIPS_B32            H5T_STD_B32BE
#define H5T_MIPS_B64            H5T_STD_B64BE
#define H5T_MIPS_F32            H5T_IEEE_F32BE
#define H5T_MIPS_F64            H5T_IEEE_F64BE

/*
 * The predefined native types. These are the types detected by H5detect and
 * they violate the naming scheme a little.  Instead of a class name,
 * precision and byte order as the last component, they have a C-like type
 * name.  If the type begins with `U' then it is the unsigned version of the
 * integer type; other integer types are signed.  The type LLONG corresponds
 * to C's `long_long' and LDOUBLE is `long double' (these types might be the
 * same as `LONG' and `DOUBLE' respectively.
 */
#define H5T_NATIVE_CHAR         (CHAR_MIN?H5T_NATIVE_SCHAR:H5T_NATIVE_UCHAR)
#define H5T_NATIVE_SCHAR        (H5open(), H5T_NATIVE_SCHAR_g)
#define H5T_NATIVE_UCHAR        (H5open(), H5T_NATIVE_UCHAR_g)
#define H5T_NATIVE_SHORT        (H5open(), H5T_NATIVE_SHORT_g)
#define H5T_NATIVE_USHORT       (H5open(), H5T_NATIVE_USHORT_g)
#define H5T_NATIVE_INT          (H5open(), H5T_NATIVE_INT_g)
#define H5T_NATIVE_UINT         (H5open(), H5T_NATIVE_UINT_g)
#define H5T_NATIVE_LONG         (H5open(), H5T_NATIVE_LONG_g)
#define H5T_NATIVE_ULONG        (H5open(), H5T_NATIVE_ULONG_g)
#define H5T_NATIVE_LLONG        (H5open(), H5T_NATIVE_LLONG_g)
#define H5T_NATIVE_ULLONG       (H5open(), H5T_NATIVE_ULLONG_g)
#define H5T_NATIVE_FLOAT        (H5open(), H5T_NATIVE_FLOAT_g)
#define H5T_NATIVE_DOUBLE       (H5open(), H5T_NATIVE_DOUBLE_g)
#define H5T_NATIVE_LDOUBLE      (H5open(), H5T_NATIVE_LDOUBLE_g)
#define H5T_NATIVE_B8           (H5open(), H5T_NATIVE_B8_g)
#define H5T_NATIVE_B16          (H5open(), H5T_NATIVE_B16_g)
#define H5T_NATIVE_B32          (H5open(), H5T_NATIVE_B32_g)
#define H5T_NATIVE_B64          (H5open(), H5T_NATIVE_B64_g)
#define H5T_NATIVE_OPAQUE       (H5open(), H5T_NATIVE_OPAQUE_g)
#define H5T_NATIVE_HADDR        (H5open(), H5T_NATIVE_HADDR_g)
#define H5T_NATIVE_HSIZE        (H5open(), H5T_NATIVE_HSIZE_g)
#define H5T_NATIVE_HSSIZE       (H5open(), H5T_NATIVE_HSSIZE_g)
#define H5T_NATIVE_HERR         (H5open(), H5T_NATIVE_HERR_g)
#define H5T_NATIVE_HBOOL        (H5open(), H5T_NATIVE_HBOOL_g)
__DLLVAR__ hid_t H5T_NATIVE_SCHAR_g;
__DLLVAR__ hid_t H5T_NATIVE_UCHAR_g;
__DLLVAR__ hid_t H5T_NATIVE_SHORT_g;
__DLLVAR__ hid_t H5T_NATIVE_USHORT_g;
__DLLVAR__ hid_t H5T_NATIVE_INT_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT_g;
__DLLVAR__ hid_t H5T_NATIVE_LONG_g;
__DLLVAR__ hid_t H5T_NATIVE_ULONG_g;
__DLLVAR__ hid_t H5T_NATIVE_LLONG_g;
__DLLVAR__ hid_t H5T_NATIVE_ULLONG_g;
__DLLVAR__ hid_t H5T_NATIVE_FLOAT_g;
__DLLVAR__ hid_t H5T_NATIVE_DOUBLE_g;
__DLLVAR__ hid_t H5T_NATIVE_LDOUBLE_g;
__DLLVAR__ hid_t H5T_NATIVE_B8_g;
__DLLVAR__ hid_t H5T_NATIVE_B16_g;
__DLLVAR__ hid_t H5T_NATIVE_B32_g;
__DLLVAR__ hid_t H5T_NATIVE_B64_g;
__DLLVAR__ hid_t H5T_NATIVE_OPAQUE_g;
__DLLVAR__ hid_t H5T_NATIVE_HADDR_g;
__DLLVAR__ hid_t H5T_NATIVE_HSIZE_g;
__DLLVAR__ hid_t H5T_NATIVE_HSSIZE_g;
__DLLVAR__ hid_t H5T_NATIVE_HERR_g;
__DLLVAR__ hid_t H5T_NATIVE_HBOOL_g;

/* C9x integer types */
#define H5T_NATIVE_INT8                 (H5open(), H5T_NATIVE_INT8_g)
#define H5T_NATIVE_UINT8                (H5open(), H5T_NATIVE_UINT8_g)
#define H5T_NATIVE_INT_LEAST8           (H5open(), H5T_NATIVE_INT_LEAST8_g)
#define H5T_NATIVE_UINT_LEAST8          (H5open(), H5T_NATIVE_UINT_LEAST8_g)
#define H5T_NATIVE_INT_FAST8            (H5open(), H5T_NATIVE_INT_FAST8_g)
#define H5T_NATIVE_UINT_FAST8           (H5open(), H5T_NATIVE_UINT_FAST8_g)
__DLLVAR__ hid_t H5T_NATIVE_INT8_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT8_g;
__DLLVAR__ hid_t H5T_NATIVE_INT_LEAST8_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT_LEAST8_g;
__DLLVAR__ hid_t H5T_NATIVE_INT_FAST8_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT_FAST8_g;

#define H5T_NATIVE_INT16                (H5open(), H5T_NATIVE_INT16_g)
#define H5T_NATIVE_UINT16               (H5open(), H5T_NATIVE_UINT16_g)
#define H5T_NATIVE_INT_LEAST16          (H5open(), H5T_NATIVE_INT_LEAST16_g)
#define H5T_NATIVE_UINT_LEAST16         (H5open(), H5T_NATIVE_UINT_LEAST16_g)
#define H5T_NATIVE_INT_FAST16           (H5open(), H5T_NATIVE_INT_FAST16_g)
#define H5T_NATIVE_UINT_FAST16          (H5open(), H5T_NATIVE_UINT_FAST16_g)
__DLLVAR__ hid_t H5T_NATIVE_INT16_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT16_g;
__DLLVAR__ hid_t H5T_NATIVE_INT_LEAST16_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT_LEAST16_g;
__DLLVAR__ hid_t H5T_NATIVE_INT_FAST16_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT_FAST16_g;

#define H5T_NATIVE_INT32                (H5open(), H5T_NATIVE_INT32_g)
#define H5T_NATIVE_UINT32               (H5open(), H5T_NATIVE_UINT32_g)
#define H5T_NATIVE_INT_LEAST32          (H5open(), H5T_NATIVE_INT_LEAST32_g)
#define H5T_NATIVE_UINT_LEAST32         (H5open(), H5T_NATIVE_UINT_LEAST32_g)
#define H5T_NATIVE_INT_FAST32           (H5open(), H5T_NATIVE_INT_FAST32_g)
#define H5T_NATIVE_UINT_FAST32          (H5open(), H5T_NATIVE_UINT_FAST32_g)
__DLLVAR__ hid_t H5T_NATIVE_INT32_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT32_g;
__DLLVAR__ hid_t H5T_NATIVE_INT_LEAST32_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT_LEAST32_g;
__DLLVAR__ hid_t H5T_NATIVE_INT_FAST32_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT_FAST32_g;

#define H5T_NATIVE_INT64                (H5open(), H5T_NATIVE_INT64_g)
#define H5T_NATIVE_UINT64               (H5open(), H5T_NATIVE_UINT64_g)
#define H5T_NATIVE_INT_LEAST64          (H5open(), H5T_NATIVE_INT_LEAST64_g)
#define H5T_NATIVE_UINT_LEAST64         (H5open(), H5T_NATIVE_UINT_LEAST64_g)
#define H5T_NATIVE_INT_FAST64           (H5open(), H5T_NATIVE_INT_FAST64_g)
#define H5T_NATIVE_UINT_FAST64          (H5open(), H5T_NATIVE_UINT_FAST64_g)
__DLLVAR__ hid_t H5T_NATIVE_INT64_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT64_g;
__DLLVAR__ hid_t H5T_NATIVE_INT_LEAST64_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT_LEAST64_g;
__DLLVAR__ hid_t H5T_NATIVE_INT_FAST64_g;
__DLLVAR__ hid_t H5T_NATIVE_UINT_FAST64_g;

/* Operations defined on all data types */
__DLL__ hid_t H5Topen(hid_t loc_id, const char *name);
__DLL__ hid_t H5Tcreate(H5T_class_t type, size_t size);
__DLL__ hid_t H5Tcopy(hid_t type_id);
__DLL__ herr_t H5Tclose(hid_t type_id);
__DLL__ htri_t H5Tequal(hid_t type1_id, hid_t type2_id);
__DLL__ herr_t H5Tlock(hid_t type_id);
__DLL__ herr_t H5Tcommit(hid_t loc_id, const char *name, hid_t type_id);
__DLL__ htri_t H5Tcommitted(hid_t type_id);

/* Operations defined on compound data types */
__DLL__ herr_t H5Tinsert(hid_t parent_id, const char *name, size_t offset,
                         hid_t member_id);
#if defined(WANT_H5_V1_2_COMPAT) || defined(H5_WANT_H5_V1_2_COMPAT)
__DLL__ herr_t H5Tinsert_array(hid_t parent_id, const char *name,
                               size_t offset, int ndims, const size_t dim[],
                               const int *perm, hid_t member_id);
#endif /* WANT_H5_V1_2_COMPAT */
__DLL__ herr_t H5Tpack(hid_t type_id);

/* Operations defined on enumeration data types */
__DLL__ hid_t H5Tenum_create(hid_t base_id);
__DLL__ herr_t H5Tenum_insert(hid_t type, const char *name, void *value);
__DLL__ herr_t H5Tenum_nameof(hid_t type, void *value, char *name/*out*/,
                             size_t size);
__DLL__ herr_t H5Tenum_valueof(hid_t type, const char *name,
                              void *value/*out*/);

/* Operations defined on variable-length data types */
__DLL__ hid_t H5Tvlen_create(hid_t base_id);

/* Operations defined on array data types */
__DLL__ hid_t H5Tarray_create(hid_t base_id, int ndims,
            const hsize_t dim[/* ndims */], const int perm[/* ndims */]);
__DLL__ int H5Tget_array_ndims(hid_t type_id);
__DLL__ herr_t H5Tget_array_dims(hid_t type_id, hsize_t dims[], int perm[]);

/* Operations defined on opaque data types */
__DLL__ herr_t H5Tset_tag(hid_t type, const char *tag);
__DLL__ char *H5Tget_tag(hid_t type);

/* Querying property values */
__DLL__ hid_t H5Tget_super(hid_t type);
__DLL__ H5T_class_t H5Tget_class(hid_t type_id);
__DLL__ htri_t H5Tdetect_class(hid_t type_id, H5T_class_t cls);
__DLL__ size_t H5Tget_size(hid_t type_id);
__DLL__ H5T_order_t H5Tget_order(hid_t type_id);
__DLL__ size_t H5Tget_precision(hid_t type_id);
__DLL__ int H5Tget_offset(hid_t type_id);
__DLL__ herr_t H5Tget_pad(hid_t type_id, H5T_pad_t *lsb/*out*/,
                          H5T_pad_t *msb/*out*/);
__DLL__ H5T_sign_t H5Tget_sign(hid_t type_id);
__DLL__ herr_t H5Tget_fields(hid_t type_id, size_t *spos/*out*/,
                             size_t *epos/*out*/, size_t *esize/*out*/,
                             size_t *mpos/*out*/, size_t *msize/*out*/);
__DLL__ size_t H5Tget_ebias(hid_t type_id);
__DLL__ H5T_norm_t H5Tget_norm(hid_t type_id);
__DLL__ H5T_pad_t H5Tget_inpad(hid_t type_id);
__DLL__ H5T_str_t H5Tget_strpad(hid_t type_id);
__DLL__ int H5Tget_nmembers(hid_t type_id);
__DLL__ char *H5Tget_member_name(hid_t type_id, int membno);
__DLL__ size_t H5Tget_member_offset(hid_t type_id, int membno);
#if defined(WANT_H5_V1_2_COMPAT) || defined(H5_WANT_H5_V1_2_COMPAT)
__DLL__ int H5Tget_member_dims(hid_t type_id, int membno, size_t dims[]/*out*/,
                               int perm[]/*out*/);
#endif /* WANT_H5_V1_2_COMPAT */
__DLL__ H5T_class_t H5Tget_member_class(hid_t type_id, int membno);
__DLL__ hid_t H5Tget_member_type(hid_t type_id, int membno);
__DLL__ herr_t H5Tget_member_value(hid_t type_id, int membno,
                                   void *value/*out*/);
__DLL__ H5T_cset_t H5Tget_cset(hid_t type_id);

/* Setting property values */
__DLL__ herr_t H5Tset_size(hid_t type_id, size_t size);
__DLL__ herr_t H5Tset_order(hid_t type_id, H5T_order_t order);
__DLL__ herr_t H5Tset_precision(hid_t type_id, size_t prec);
__DLL__ herr_t H5Tset_offset(hid_t type_id, size_t offset);
__DLL__ herr_t H5Tset_pad(hid_t type_id, H5T_pad_t lsb, H5T_pad_t msb);
__DLL__ herr_t H5Tset_sign(hid_t type_id, H5T_sign_t sign);
__DLL__ herr_t H5Tset_fields(hid_t type_id, size_t spos, size_t epos,
                             size_t esize, size_t mpos, size_t msize);
__DLL__ herr_t H5Tset_ebias(hid_t type_id, size_t ebias);
__DLL__ herr_t H5Tset_norm(hid_t type_id, H5T_norm_t norm);
__DLL__ herr_t H5Tset_inpad(hid_t type_id, H5T_pad_t pad);
__DLL__ herr_t H5Tset_cset(hid_t type_id, H5T_cset_t cset);
__DLL__ herr_t H5Tset_strpad(hid_t type_id, H5T_str_t strpad);

/* Type conversion database */
__DLL__ herr_t H5Tregister(H5T_pers_t pers, const char *name, hid_t src_id,
                           hid_t dst_id, H5T_conv_t func);
__DLL__ herr_t H5Tunregister(H5T_pers_t pers, const char *name, hid_t src_id,
                             hid_t dst_id, H5T_conv_t func);
__DLL__ H5T_conv_t H5Tfind(hid_t src_id, hid_t dst_id, H5T_cdata_t **pcdata);
__DLL__ herr_t H5Tconvert(hid_t src_id, hid_t dst_id, hsize_t nelmts,
                          void *buf, void *background, hid_t plist_id);
__DLL__ H5T_overflow_t H5Tget_overflow(void);
__DLL__ herr_t H5Tset_overflow(H5T_overflow_t func);

#ifdef __cplusplus
}
#endif
#endif
